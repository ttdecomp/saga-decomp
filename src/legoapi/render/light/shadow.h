#pragma once

// Shadow light system (module legoapi/render/light, shadow.cpp).

void InitShadowLights();
float BlobShadowFade(struct nuvec_s *position, float fade_start, float fade_end, float alpha);

extern "C" int EShadowInfo();
extern "C" int ShadowInfo();
