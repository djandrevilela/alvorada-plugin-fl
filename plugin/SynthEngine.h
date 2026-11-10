#pragma once
// Motor do synth do Alvorada: subtrativo (osciladores + unison + filtro), FM (teclas/sinos) e cordas (Karplus-Strong).
#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <cmath>
#include <vector>

#include "Presets.h"

struct PartState {
    std::atomic<int> preset{0};
    std::atomic<float> bright{0.6f}, attack{0.1f}, tail{0.5f}, width{0.5f}, sub{0.f};
};

struct AlvSound : juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class AlvVoice : public juce::SynthesiserVoice {
public:
    static constexpr int kMaxUni = 7;

    explicit AlvVoice(const PartState& s) : ps(s), ks((size_t) 8192, 0.f) {}

    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<AlvSound*>(s) != nullptr; }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void startNote(int note, float vel, juce::SynthesiserSound*, int) override {
        const auto& all = alvorada::presets();
        P = &all[(size_t) juce::jlimit(0, (int) all.size() - 1, ps.preset.load())];
        sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
        freq = juce::MidiMessage::getMidiNoteInHertz(note);
        level = 0.2f + 0.8f * vel;
        const float att = ps.attack.load(), tl = ps.tail.load(), wd = ps.width.load();

        uni = juce::jlimit(1, kMaxUni, P->unison);
        for (int u = 0; u < uni; ++u) {
            float x = uni > 1 ? ((float) u - 0.5f * (uni - 1)) / (0.5f * (uni - 1)) : 0.f;
            det[u] = std::pow(2.f, x * wd * P->maxDetune / 1200.f);
            ph1[u] = std::fmod(0.371f * (float) u + 0.113f, 1.f);
            ph2[u] = std::fmod(0.529f * (float) u + 0.271f, 1.f);
            float pan = 0.5f + 0.45f * wd * x;
            gL[u] = std::cos(pan * juce::MathConstants<float>::halfPi);
            gR[u] = std::sin(pan * juce::MathConstants<float>::halfPi);
        }
        norm = 0.55f / std::sqrt((float) uni);
        ratio2 = std::pow(2.f, P->semi2 / 12.f);
        subPh = 0.f;
        fmEnv = 1.f;
        svfL.reset(); svfR.reset();

        juce::ADSR::Parameters ap;
        ap.attack = alvorada::macroAttackSec(att);
        ap.decay = P->engine == alvorada::Engine::String ? 0.1f : alvorada::macroDecaySec(tl);
        ap.sustain = P->engine == alvorada::Engine::String ? 1.f : P->sustain;
        ap.release = alvorada::macroReleaseSec(tl);
        amp.setSampleRate(sr);
        amp.setParameters(ap);
        amp.noteOn();
        juce::ADSR::Parameters fp;
        fp.attack = 0.001f;
        fp.decay = juce::jmax(0.02f, P->fdecay * (0.4f + 1.2f * tl));
        fp.sustain = 0.f;
        fp.release = 0.2f;
        fenvAdsr.setSampleRate(sr);
        fenvAdsr.setParameters(fp);
        fenvAdsr.noteOn();
        fmDecayCoef = std::exp(-1.f / (float) (sr * juce::jmax(0.03f, P->fmDecay * (0.3f + 2.f * tl))));

        if (P->engine == alvorada::Engine::String) initString(ps.bright.load());
    }

    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) { amp.noteOff(); fenvAdsr.noteOff(); }
        else { amp.reset(); clearCurrentNote(); }
    }

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override {
        if (!amp.isActive()) { clearCurrentNote(); return; }
        float* L = out.getWritePointer(0, start);
        float* R = out.getNumChannels() > 1 ? out.getWritePointer(1, start) : nullptr;
        const float bright = ps.bright.load();
        const float subLevel = ps.sub.load();
        switch (P->engine) {
            case alvorada::Engine::Subtractive: renderSub(L, R, num, bright, subLevel); break;
            case alvorada::Engine::FM: renderFM(L, R, num, bright); break;
            case alvorada::Engine::String: renderString(L, R, num); break;
        }
        if (!amp.isActive()) clearCurrentNote();
    }

private:
    struct SVF {
        float s1 = 0, s2 = 0, a1 = 1, a2 = 0, a3 = 0;
        void reset() { s1 = s2 = 0; }
        void set(float fc, double sr, float res) {
            fc = juce::jlimit(20.f, (float) (0.45 * sr), fc);
            float g = std::tan(juce::MathConstants<float>::pi * fc / (float) sr);
            float k = 2.f - 1.9f * juce::jlimit(0.f, 0.97f, res);
            a1 = 1.f / (1.f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        float lp(float v0) {
            float v3 = v0 - s2;
            float v1 = a1 * s1 + a2 * v3;
            float v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2.f * v1 - s1;
            s2 = 2.f * v2 - s2;
            return v2;
        }
    };

    static float blep(float t, float dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }

    static float osc(alvorada::Wave w, float& ph, float dt) {
        float v = 0.f;
        switch (w) {
            case alvorada::Wave::Saw: v = 2.f * ph - 1.f; v -= blep(ph, dt); break;
            case alvorada::Wave::Square: {
                v = ph < 0.5f ? 1.f : -1.f;
                v += blep(ph, dt);
                float p2 = ph + 0.5f; if (p2 >= 1.f) p2 -= 1.f;
                v -= blep(p2, dt);
                break;
            }
            case alvorada::Wave::Tri: v = 4.f * std::abs(ph - 0.5f) - 1.f; break;
            case alvorada::Wave::Sine: v = std::sin(juce::MathConstants<float>::twoPi * ph); break;
        }
        ph += dt;
        if (ph >= 1.f) ph -= 1.f;
        return v;
    }

    void renderSub(float* L, float* R, int num, float bright, float subLevel) {
        const float base = alvorada::macroCutoffHz(bright);
        const bool filtered = !(P->w1 == alvorada::Wave::Sine && P->mix2 <= 0.f && P->fenv <= 0.f && P->resonance <= 0.f && uni == 1);
        for (int i = 0; i < num; ++i) {
            const float fe = fenvAdsr.getNextSample();
            if ((i & 15) == 0 && filtered) {
                float cut = base * std::pow(2.f, P->fenv * 4.f * fe);
                svfL.set(cut, sr, P->resonance);
                svfR.a1 = svfL.a1; svfR.a2 = svfL.a2; svfR.a3 = svfL.a3;  // mesmos coeficientes, estados separados
            }
            float l = 0.f, r = 0.f;
            for (int u = 0; u < uni; ++u) {
                float dt = (float) (freq * det[u] / sr);
                float s = osc(P->w1, ph1[u], dt);
                if (P->mix2 > 0.f) s += P->mix2 * osc(P->w2, ph2[u], dt * ratio2);
                l += s * gL[u];
                r += s * gR[u];
            }
            l *= norm; r *= norm;
            if (subLevel > 0.f) {
                float ss = std::sin(juce::MathConstants<float>::twoPi * subPh) * subLevel * 0.7f;
                subPh += (float) (0.5 * freq / sr);
                if (subPh >= 1.f) subPh -= 1.f;
                l += ss; r += ss;
            }
            if (filtered) { l = svfL.lp(l); r = svfR.lp(r); }
            const float a = amp.getNextSample() * level;
            L[i] += l * a;
            if (R) R[i] += r * a;
        }
    }

    void renderFM(float* L, float* R, int num, float bright) {
        const float index = P->fmIndex * (0.3f + 1.4f * bright);
        const float tp = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < num; ++i) {
            float l = 0.f, r = 0.f;
            const float idx = index * fmEnv;
            fmEnv *= fmDecayCoef;
            for (int u = 0; u < uni; ++u) {
                float dc = (float) (freq * det[u] / sr);
                float mod = std::sin(tp * ph2[u]);
                float s = std::sin(tp * ph1[u] + idx * mod);
                ph1[u] += dc; if (ph1[u] >= 1.f) ph1[u] -= 1.f;
                ph2[u] += dc * P->fmRatio; if (ph2[u] >= 1.f) ph2[u] -= std::floor(ph2[u]);
                l += s * gL[u]; r += s * gR[u];
            }
            const float a = amp.getNextSample() * level * norm * 1.5f;
            L[i] += l * a;
            if (R) R[i] += r * a;
        }
    }

    void initString(float bright) {
        const float period = (float) (sr / freq);
        const float tuned = juce::jmax(2.5f, period - 0.5f);
        ksN = juce::jlimit(2, 8190, (int) std::floor(tuned));
        const float frac = tuned - (float) ksN;
        apCoef = (1.f - frac) / (1.f + frac);
        apZ = 0.f; ksPrev = 0.f; ksPos = 0;
        damp = juce::jlimit(0.05f, 0.95f, 0.95f - 0.8f * bright);
        const float tl = ps.tail.load();
        rho = 1.f - 0.005f * std::pow(0.01f, tl);
        // excitação: ruído de semente fixa (LCG) filtrado em função do brilho
        uint32_t seed = 12345u + (uint32_t) std::lround(freq * 10.0);
        float lpv = 0.f;
        const float k = 0.25f + 0.7f * bright;
        for (int i = 0; i < ksN; ++i) {
            seed = seed * 1664525u + 1013904223u;
            float n = ((float) (seed >> 8) / 8388608.f) - 1.f;
            lpv += k * (n - lpv);
            ks[(size_t) i] = lpv * 1.6f;
        }
    }

    void renderString(float* L, float* R, int num) {
        for (int i = 0; i < num; ++i) {
            float y = ks[(size_t) ksPos];
            float f = y + damp * (0.5f * (y + ksPrev) - y);
            ksPrev = y;
            float fa = apCoef * f + apZ;
            apZ = f - apCoef * fa;
            ks[(size_t) ksPos] = fa * rho;
            if (++ksPos >= ksN) ksPos = 0;
            const float a = amp.getNextSample() * level * 0.8f;
            L[i] += y * a;
            if (R) R[i] += y * a;
        }
    }

    const PartState& ps;
    const alvorada::SynthPreset* P = nullptr;
    double sr = 44100.0, freq = 440.0;
    float level = 1.f, norm = 0.5f, ratio2 = 1.f, subPh = 0.f;
    int uni = 1;
    float det[kMaxUni]{}, ph1[kMaxUni]{}, ph2[kMaxUni]{}, gL[kMaxUni]{}, gR[kMaxUni]{};
    juce::ADSR amp, fenvAdsr;
    SVF svfL, svfR;
    float fmEnv = 1.f, fmDecayCoef = 0.9999f;
    // Karplus-Strong
    std::vector<float> ks;
    int ksN = 100, ksPos = 0;
    float ksPrev = 0.f, apCoef = 0.f, apZ = 0.f, damp = 0.5f, rho = 0.999f;
};
