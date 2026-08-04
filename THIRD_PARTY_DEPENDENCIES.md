# Third-party dependencies inventory

This file lists third-party C/C++ dependencies verified in the repository. Evidence sources: CMakeLists, docs, .github workflows, and files under third_party/.

| Component | Version | Source URL | License | Integration Method |
|---|---|---|---|---|
| Qt (Qt Toolkit / qtbase) | 6.4.2 | https://github.com/qt/qtbase | GNU LGPLv3 / GNU GPLv3 (see third_party/Qt/LICENSE.*) | Linked via find_package(Qt...) in CMakeLists.txt; CI installs with aqt / install-qt action |
| pugixml | 1.15 (referenced in docs as required when FMU support enabled) | https://github.com/zeux/pugixml (release v1.15) | MIT (third_party/pugixml/LICENSE.md) | External library: CMake option -DPUGIXML_SOURCE_PATH to point to local pugixml source; code includes "pugixml.hpp" |
| FMI Standard (headers) | v2.0.4 and v3.0.1 (referenced in docs as required for FMU support) | https://github.com/modelica/fmi-standard (releases) | BSD-2-Clause (project headers, per upstream repo) | External headers; CMake expects -DFMI2_SOURCE_PATH / -DFMI3_SOURCE_PATH to point to FMI headers |
| esmini | v2.31.1 | https://github.com/esmini/esmini | Mozilla Public License 2.0 (third_party/esmini/LICENSE.txt) | Optional sample integration: sample links to external esmini library via ESMINI_LIBRARY_PATH; repository includes vendored LICENSE but not source |
| pytest | 9.1.1 | https://github.com/pytest-dev/pytest | MIT (upstream LICENSE) | Test dependency: pinned in requirements.txt (pytest==9.1.1); used by CI to run tests (pytest -vv) |
| aqtinstall | 3.3.0 | https://github.com/miurahr/aqtinstall | MIT (upstream project states MIT) | CI helper: pinned in CI workflow (pip install aqtinstall==3.3.0) to fetch/install Qt for CI builds |

## License copies present in repo (indicative of vendoring/licensing notes)
- third_party/Qt/LICENSE.LGPLv3 and third_party/Qt/LICENSE.GPL3 — Qt licensing texts included
- third_party/pugixml/LICENSE.md — MIT license for pugixml
- third_party/esmini/LICENSE.txt — MPL-2.0 license text present

---
Last updated: 2026-08-04
