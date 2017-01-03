### Overview

Approxilyzer is an open-source framework for instruction level approximation and resiliency. Approxilyzer provides a systematic way to identify instructions that exhibit first-order approximation potential. It can also identify silent data corruption (SDC) causing instructions in the presence of single-bit errors. Approxilyzer employs static and dynamic analysis, in addition to heuristics, to reduce the run-time of finding Approximate instructions and SDC-causing instructions by 3-6x orders of magnitude.

### Publications
Please cite our papers when you publish results that you have obtained using Approxilyzer:

1. Approxilyzer: Towards A Systematic Framework for Instruction-Level Approximate Computing and its Application to Hardware Resiliency [[PDF](http://rsim.cs.uiuc.edu/Pubs/16-MICRO-approxilyzer.pdf)]

2. Relyzer: Exploiting Application-Level Fault Equivalence to Analyze Application Resiliency to Transient Faults  [[PDF](http://rsim.cs.illinois.edu/Pubs/12-ASPLOS-Hari.pdf)]


### Downloading and License Agreement
Approxilyzer is available at no cost with the [University of Illinois/NCSA open source license agreement](http://rsim.cs.illinois.edu/approxilyzer/LICENSE.txt).

We request that you [register here](https://docs.google.com/forms/d/e/1FAIpQLSfeCpA3me2hP-TvONTTRBLWYfD-RWzlSe38bjHln8eEqIfPJw/viewform) before you start using Approxilyzer.

To install and run Approxilyzer, download the code from github, and follow the instructions in the README.txt.

### Authors and Contributors
Approxilyzer builds upon Relyzer as an underlying tool that systematically analyzes and performs selective fault injections for transient faults.

The primary contributors to these projects are [Prof. Sarita V Adve](http://rsim.cs.illinois.edu/~sadve/) (UIUC), [Siva Kumar Sastry Hari](https://research.nvidia.com/users/siva-hari) (UIUC, now at NVIDIA Research), [Abdulrahman Mahmoud](http://web.engr.illinois.edu/~amahmou2/) (UIUC), Helia Naeimi (Intel), Pradeep Ramachandran (UIUC, now at Multicoreware), and Radha Venkatagiri (UIUC). 

### Funding Sources
The development of Approxilyzer and Relyzer is funded in part by the National Science Foundation
under Grants CCF-1320941 and CCF-0811693, by the Center for Future Architectures
Research (C-FAR), one of six centers of STARnet, a Semiconductor Research
Corporation program sponsored by MARCO and DARPA, and by the Gigascale Systems Research Center
(funded under FCRP, an SRC program). Pradeep Ramachandran was supported by
an Intel PhD fellowship and an IBM PhD scholarship. 

### Support or Contact
For more information, see the [RSIM Group home page](http://rsim.cs.illinois.edu/index.html). To report bugs or contributions, please contact @ma3mool on GitHub.

