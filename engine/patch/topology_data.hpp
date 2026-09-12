#pragma once

// Compiled recipe contracts; optional connections precede required ones.
namespace drumfoundry {
inline constexpr const char* RecipeTopologyJson = R"json(
[
  {
    "recipe": "metal.cymbal.v1",
    "nodes": [
      {"id":"contact","type":"exciter.contact","version":1},
      {"id":"body","type":"body.stochastic-modal-field","version":1},
      {"id":"observation","type":"observation.dual-source","version":1},
      {"id":"output","type":"output.mono","version":1}
    ],
    "connections": [
      {"from":"contact.body","to":"body.primary","required":false},
      {"from":"contact.direct","to":"observation.direct","required":false},
      {"from":"body.audio","to":"observation.body","required":false},
      {"from":"observation.audio","to":"output.audio","required":true}
    ]
  },
  {
    "recipe": "drum.kick.v1",
    "nodes": [
      {"id":"kick-contact","type":"exciter.contact","version":1},
      {"id":"kick-thump","type":"exciter.thump","version":1},
      {"id":"kick-resonance","type":"body.membrane-modal","version":1},
      {"id":"kick-tension","type":"interaction.strike-energy","version":1},
      {"id":"kick-mix","type":"transform.sum3","version":1},
      {"id":"kick-observation","type":"observation.equalizer","version":1},
      {"id":"kick-output","type":"output.mono","version":1}
    ],
    "connections": [
      {"from":"kick-contact.direct","to":"kick-mix.a","required":false},
      {"from":"kick-thump.audio","to":"kick-mix.b","required":false},
      {"from":"kick-resonance.audio","to":"kick-mix.c","required":false},
      {"from":"kick-contact.body","to":"kick-resonance.drive","required":true},
      {"from":"kick-contact.event","to":"kick-tension.strike","required":true},
      {"from":"kick-tension.tension","to":"kick-resonance.tension","required":true},
      {"from":"kick-mix.audio","to":"kick-observation.audio","required":true},
      {"from":"kick-observation.audio","to":"kick-output.audio","required":true}
    ]
  },
  {
    "recipe": "drum.membrane.v1",
    "nodes": [
      {"id":"membrane-contact","type":"exciter.contact","version":1},
      {"id":"membrane-fm","type":"exciter.correlated-fm","version":1},
      {"id":"membrane-direct-mix","type":"transform.sum2","version":1},
      {"id":"membrane-body-mix","type":"transform.sum2","version":1},
      {"id":"membrane-tension","type":"interaction.strike-energy","version":1},
      {"id":"membrane-body","type":"body.membrane-modal","version":1},
      {"id":"membrane-observation","type":"observation.dual-source","version":1},
      {"id":"membrane-eq","type":"observation.equalizer","version":1},
      {"id":"membrane-output","type":"output.mono","version":1}
    ],
    "connections": [
      {"from":"membrane-contact.direct","to":"membrane-direct-mix.a","required":false},
      {"from":"membrane-contact.body","to":"membrane-body-mix.a","required":false},
      {"from":"membrane-fm.audio","to":"membrane-direct-mix.b","required":false},
      {"from":"membrane-fm.audio","to":"membrane-body-mix.b","required":false},
      {"from":"membrane-body.audio","to":"membrane-observation.body","required":false},
      {"from":"membrane-contact.event","to":"membrane-tension.strike","required":true},
      {"from":"membrane-body-mix.audio","to":"membrane-body.drive","required":true},
      {"from":"membrane-tension.tension","to":"membrane-body.tension","required":true},
      {"from":"membrane-direct-mix.audio","to":"membrane-observation.direct","required":true},
      {"from":"membrane-observation.audio","to":"membrane-eq.audio","required":true},
      {"from":"membrane-eq.audio","to":"membrane-output.audio","required":true}
    ]
  },
  {
    "recipe": "drum.snare.v1",
    "nodes": [
      {"id":"membrane-contact","type":"exciter.contact","version":1},
      {"id":"membrane-fm","type":"exciter.correlated-fm","version":1},
      {"id":"membrane-direct-mix","type":"transform.sum2","version":1},
      {"id":"membrane-body-mix","type":"transform.sum2","version":1},
      {"id":"membrane-tension","type":"interaction.strike-energy","version":1},
      {"id":"membrane-body","type":"body.membrane-modal","version":1},
      {"id":"snare-wires","type":"interaction.wire-rack","version":1},
      {"id":"membrane-observation","type":"observation.three-source","version":1},
      {"id":"membrane-eq","type":"observation.equalizer","version":1},
      {"id":"membrane-output","type":"output.mono","version":1}
    ],
    "connections": [
      {"from":"membrane-contact.direct","to":"membrane-direct-mix.a","required":false},
      {"from":"membrane-contact.body","to":"membrane-body-mix.a","required":false},
      {"from":"membrane-fm.audio","to":"membrane-direct-mix.b","required":false},
      {"from":"membrane-fm.audio","to":"membrane-body-mix.b","required":false},
      {"from":"membrane-body.audio","to":"membrane-observation.body","required":false},
      {"from":"membrane-body.audio","to":"snare-wires.motion","required":false},
      {"from":"snare-wires.audio","to":"membrane-observation.wires","required":false},
      {"from":"membrane-contact.event","to":"membrane-tension.strike","required":true},
      {"from":"membrane-body-mix.audio","to":"membrane-body.drive","required":true},
      {"from":"membrane-tension.tension","to":"membrane-body.tension","required":true},
      {"from":"membrane-direct-mix.audio","to":"membrane-observation.direct","required":true},
      {"from":"membrane-observation.audio","to":"membrane-eq.audio","required":true},
      {"from":"membrane-eq.audio","to":"membrane-output.audio","required":true}
    ]
  }
]
)json";
}
