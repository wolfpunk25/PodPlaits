#pragma once
#include <stdint.h>
struct MoodPreset {const char* name;const char* description;uint8_t model;bool drone;uint16_t values[9];};
// Harmonics, timbre, morph, root, decay, rate, reverb, drift, volume (0..1000).
static const MoodPreset moods[]={
 {"Warm Drift","Slow, rounded chords",14,false,{280,260,380,333,850,95,600,230,400}},
 {"Glass Garden","Gentle ringing notes",20,false,{360,420,530,667,800,210,680,170,350}},
 {"Night Tide","Dark, moving waves",8,true,{180,200,320,167,900,70,650,420,400}},
 {"Floating Choir","Airy, changing vowels",11,true,{430,390,600,500,900,80,720,320,350}},
 {"Soft Sparks","Sparse, plucked strings",19,false,{320,280,480,500,650,160,650,180,380}},
 {"Deep Space","Low, shifting harmonics",12,true,{180,300,420,0,950,50,780,360,380}},
};
static const unsigned moodCount=sizeof(moods)/sizeof(moods[0]);
