#include <cmath>

#include "Presets.h"

namespace alvorada {

const std::vector<SynthPreset>& presets() {
    using E = Engine;
    using W = Wave;
    //                category   name                  engine      w1        w2        semi2 mix2 uni det   res   fenv  fdec  sus   fmR   fmI  fmD    bri   att   tail  wid   sub   vol
    static const std::vector<SynthPreset> p = {
        {"Lead",    "Supersaw Lead",      E::Subtractive, W::Saw,    W::Saw,    0.f,  0.f, 7, 28.f, 0.15f, 0.20f, 0.5f, 0.90f, 0.f, 0.f, 0.f,  0.74f, 0.05f, 0.45f, 0.70f, 0.00f, 0.75f},
        {"Lead",    "Soft Lead",          E::Subtractive, W::Tri,    W::Saw,    12.f, 0.35f, 3, 10.f, 0.10f, 0.10f, 0.5f, 0.90f, 0.f, 0.f, 0.f,  0.58f, 0.12f, 0.50f, 0.40f, 0.00f, 0.80f},
        {"Lead",    "Square Lead",        E::Subtractive, W::Square, W::Square, 0.f,  0.f, 3, 12.f, 0.20f, 0.30f, 0.4f, 0.85f, 0.f, 0.f, 0.f,  0.62f, 0.05f, 0.40f, 0.45f, 0.00f, 0.70f},
        {"Lead",    "Bright Saw Lead",    E::Subtractive, W::Saw,    W::Square, 0.f,  0.5f, 5, 18.f, 0.20f, 0.25f, 0.4f, 0.90f, 0.f, 0.f, 0.f,  0.86f, 0.04f, 0.40f, 0.55f, 0.00f, 0.70f},
        {"Pluck",   "Future Pluck",       E::Subtractive, W::Saw,    W::Saw,    0.f,  0.f, 5, 22.f, 0.25f, 0.70f, 0.22f, 0.00f, 0.f, 0.f, 0.f, 0.50f, 0.02f, 0.38f, 0.60f, 0.00f, 0.80f},
        {"Pluck",   "Short Stab",         E::Subtractive, W::Saw,    W::Square, 0.f,  0.4f, 3, 14.f, 0.20f, 0.60f, 0.12f, 0.00f, 0.f, 0.f, 0.f, 0.55f, 0.02f, 0.25f, 0.45f, 0.00f, 0.80f},
        {"Pluck",   "Bell Pluck",         E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 2, 6.f,  0.f,   0.f,   0.f,   0.00f, 3.5f, 3.5f, 0.5f, 0.50f, 0.02f, 0.55f, 0.40f, 0.00f, 0.65f},
        {"Pluck",   "Marimba",            E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.f,   0.f,   0.f,   0.00f, 4.0f, 1.4f, 0.18f, 0.45f, 0.01f, 0.35f, 0.15f, 0.00f, 0.70f},
        {"Pad",     "Warm Pad",           E::Subtractive, W::Saw,    W::Tri,    0.f,  0.5f, 5, 18.f, 0.10f, 0.00f, 0.5f, 1.00f, 0.f, 0.f, 0.f,  0.35f, 0.62f, 0.70f, 0.80f, 0.00f, 0.70f},
        {"Pad",     "Glass Pad",          E::Subtractive, W::Sine,   W::Tri,    12.f, 0.5f, 4, 12.f, 0.05f, 0.00f, 0.5f, 1.00f, 0.f, 0.f, 0.f,  0.62f, 0.58f, 0.72f, 0.70f, 0.00f, 0.65f},
        {"Pad",     "Air Pad",            E::Subtractive, W::Tri,    W::Saw,    7.f,  0.4f, 4, 14.f, 0.10f, 0.10f, 0.6f, 1.00f, 0.f, 0.f, 0.f,  0.45f, 0.66f, 0.75f, 0.85f, 0.00f, 0.65f},
        {"Teclas",  "Soft Piano",         E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 2, 5.f,  0.f,   0.f,   0.f,   0.06f, 1.0f, 2.2f, 0.45f, 0.45f, 0.01f, 0.62f, 0.25f, 0.00f, 0.60f},
        {"Teclas",  "Electric Piano",     E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 2, 9.f,  0.f,   0.f,   0.f,   0.45f, 1.0f, 2.6f, 0.9f,  0.50f, 0.01f, 0.55f, 0.50f, 0.00f, 0.55f},
        {"Teclas",  "Bright Piano",       E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 2, 5.f,  0.f,   0.f,   0.f,   0.05f, 2.0f, 3.0f, 0.35f, 0.62f, 0.01f, 0.58f, 0.25f, 0.00f, 0.62f},
        {"Teclas",  "Drawbar Organ",      E::Subtractive, W::Sine,   W::Square, 12.f, 0.5f, 2, 6.f,  0.00f, 0.00f, 0.5f, 1.00f, 0.f, 0.f, 0.f,  0.52f, 0.02f, 0.30f, 0.35f, 0.00f, 0.70f},
        {"Baixo",   "Sub Sine",           E::Subtractive, W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.00f, 0.00f, 0.5f, 1.00f, 0.f, 0.f, 0.f,  0.35f, 0.04f, 0.32f, 0.00f, 0.30f, 0.65f},
        {"Baixo",   "Deep Sub",           E::Subtractive, W::Sine,   W::Tri,    0.f,  0.3f, 1, 0.f,  0.05f, 0.00f, 0.5f, 1.00f, 0.f, 0.f, 0.f,  0.40f, 0.05f, 0.38f, 0.00f, 0.50f, 0.65f},
        {"Baixo",   "Saw Bass",           E::Subtractive, W::Saw,    W::Saw,    0.f,  0.f, 2, 6.f,  0.30f, 0.50f, 0.2f, 0.80f, 0.f, 0.f, 0.f,  0.35f, 0.02f, 0.30f, 0.20f, 0.40f, 0.80f},
        {"Baixo",   "Rolling Bass",       E::Subtractive, W::Square, W::Saw,    0.f,  0.4f, 2, 6.f,  0.30f, 0.60f, 0.12f, 0.60f, 0.f, 0.f, 0.f, 0.30f, 0.01f, 0.20f, 0.15f, 0.40f, 0.80f},
        {"Baixo",   "Wide Future Bass",   E::Subtractive, W::Saw,    W::Saw,    0.f,  0.f, 7, 25.f, 0.20f, 0.30f, 0.4f, 0.90f, 0.f, 0.f, 0.f,  0.50f, 0.03f, 0.40f, 0.60f, 0.30f, 0.75f},
        {"Guitarra","Nylon Guitar",       E::String,      W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.f,   0.f,   0.f,   1.00f, 0.f, 0.f, 0.f,  0.35f, 0.01f, 0.55f, 0.20f, 0.00f, 0.60f},
        {"Guitarra","Steel Guitar",       E::String,      W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.f,   0.f,   0.f,   1.00f, 0.f, 0.f, 0.f,  0.65f, 0.01f, 0.60f, 0.20f, 0.00f, 0.58f},
        {"Guitarra","Muted Pluck",        E::String,      W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.f,   0.f,   0.f,   1.00f, 0.f, 0.f, 0.f,  0.25f, 0.01f, 0.28f, 0.10f, 0.00f, 0.62f},
        {"Guitarra","Harp Pluck",         E::String,      W::Sine,   W::Sine,   0.f,  0.f, 1, 0.f,  0.f,   0.f,   0.f,   1.00f, 0.f, 0.f, 0.f,  0.72f, 0.01f, 0.78f, 0.30f, 0.00f, 0.58f},
        {"Arp",     "Arp Pluck",          E::Subtractive, W::Saw,    W::Square, 0.f,  0.4f, 3, 12.f, 0.25f, 0.60f, 0.15f, 0.00f, 0.f, 0.f, 0.f,  0.52f, 0.01f, 0.30f, 0.50f, 0.00f, 0.80f},
        {"Arp",     "Glass Arp",          E::FM,          W::Sine,   W::Sine,   0.f,  0.f, 2, 8.f,  0.f,   0.f,   0.f,   0.00f, 2.0f, 2.0f, 0.3f,  0.55f, 0.01f, 0.35f, 0.50f, 0.00f, 0.65f},
    };
    return p;
}

int presetIndexByName(const std::string& name) {
    const auto& p = presets();
    for (size_t i = 0; i < p.size(); ++i)
        if (name == p[i].name) return (int) i;
    return -1;
}

}  // namespace alvorada
