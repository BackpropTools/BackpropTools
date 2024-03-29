<div align="center">
  <center><h1><span style="color:#7DB9B6">BackpropTools</span>: A Fast, Portable Deep Reinforcement Learning Library for Continuous Control</h1></center>
</div>

<p align="center">
  <a href="https://arxiv.org/abs/2306.03530">Paper on arXiv</a> | <a href="https://backprop.tools">Live demo (browser)</a> 
  </br>
  <a href="https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=01-Containers.ipynb">
  <img src="https://mybinder.org/badge_logo.svg" alt="Run tutorials on Binder">
  </a>
  <a href="https://backproptools.readthedocs.io">
  <img src="https://img.shields.io/badge/Documentation-Read%20the%20Docs-blue.svg" alt="Documentation">
  </a>
</p>



<div align="center">
<img src="https://github.com/BackpropTools/media/blob/master/pendulum_v1_inference.gif" alt="animated" height='200'/>
</div>
<div align="center">
    Trained on a 2020 MacBook Pro (M1) using <span style="color:#7DB9B6">BackpropTools</span> TD3
</div>
</br>
<div align="center">
<img src="https://github.com/BackpropTools/media/blob/master/backprop_tools_mujoco_ant_ppo.gif" alt="animated" height='300'/>  
</div>

<div align="center">
    Trained on a 2020 MacBook Pro (M1) using <span style="color:#7DB9B6">BackpropTools</span> PPO
</div>





## Content
- [Citing](#citing)
- [Getting Started](#getting-started)
  - [Running Prebuilt Binaries](#running-prebuilt-binaries)
  - [Cloning the Repository](#cloning-the-repository)
  - [Docker](#docker)
  - [Native](#native)
    - [Unix (Linux and macOS)](#unix-linux-and-macos)
    - [Windows](#windows)
- [Embedded Platforms](#embedded-platforms)
- [Naming Convention](#naming-convention)
## Citing
When using BackpropTools in an academic work please cite our publication using the following Bibtex citation:
```
@misc{eschmann2023backproptools,
      title={BackpropTools: A Fast, Portable Deep Reinforcement Learning Library for Continuous Control}, 
      author={Jonas Eschmann and Dario Albani and Giuseppe Loianno},
      year={2023},
      eprint={2306.03530},
      archivePrefix={arXiv},
      primaryClass={cs.LG}
}
```
## Getting Started
The getting started documentation is divided in two parts: a tutorial on how BackpropTools works internally and replication instructions for the results from the paper. 
### Tutorial on BackpropTools internals
| Chapter | Name | Documentation | Interactive Notebook |
| ------- | ---- | ------------  | ---- |
| 0 | Overview                    | [readthedocs](https://backproptools.readthedocs.io/en/latest/overview.html)                           | -
| 1 | Containers                  | [readthedocs](https://backproptools.readthedocs.io/en/latest/01-Containers.html)                      | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=01-Containers.ipynb) | 
| 2 | Multiple Dispatch           | [readthedocs](https://backproptools.readthedocs.io/en/latest/02-Multiple%20Dispatch.html)             | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=02-Multiple%20Dispatch.ipynb) | 
| 3 | Deep Learning               | [readthedocs](https://backproptools.readthedocs.io/en/latest/03-Deep%20Learning.html)                 | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=03-Deep%20Learning.ipynb) | 
| 4 | CPU Acceleration            | [readthedocs](https://backproptools.readthedocs.io/en/latest/04-CPU%20Acceleration.html)              | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=04-CPU%20Acceleration.ipynb) | 
| 5 | MNIST Classification        | [readthedocs](https://backproptools.readthedocs.io/en/latest/05-MNIST%20Classification.html)          | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=05-MNIST%20Classification.ipynb) | 
| 6 | Deep Reinforcement Learning | [readthedocs](https://backproptools.readthedocs.io/en/latest/06-Deep%20Reinforcement%20Learning.html) | [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/BackpropTools/documentation/binder?labpath=06-Deep%20Reinforcement%20Learning.ipynb) | 
### Running Prebuilt Binaries
For easy replication we provide prebuilt, standalone binaries for macOS, Linux and Windows. Just download the [latest release](https://github.com/BackpropTools/BackpropTools/releases/) for your platform and run the executables out of the box without installing any dependencies. 

Note: The macOS and Windows binaries should work out of the box for the versions: macOS >= 12 and Windows >= 10. 

The Linux binaries should work on distributions with a not too old libstdc++ (e.g. Ubuntu >= 20.04 or up to date Arch Linux). 

The Linux binaries can also be run under WSL2 on Windows 10 (even including GPU acceleration). 

When running the Linux binaries on a headless version of Ubuntu (e.g. inside Docker or WSL) the training should work out of the box but additional dependencies are required for running the MuJoCo user interface to visualize the policy after training. The instructions to install the aforementioned dependencies is in the `readme_linux.txt` of the Linux release. 

### Cloning the repository
To build the examples from source (either in Docker or natively), first the repository should be cloned.
Instead of cloning all submodules using `git clone --recursive` which takes a lot of space and bandwidth we recommend cloning the main repo containing all the standalone code for `BackpropTools` and then cloning the required sets of submodules later:
```
git clone https://github.com/BackpropTools/BackpropTools.git
```
#### Cloning submodules
There are three classes of submodules:
1. External dependencies (in `external/`)
   * E.g. HDF5 for checkpointing, Tensorboard for logging, or MuJoCo for the simulation of contact dynamics
2. Examples/Code for embedded platforms (in `embedded_platforms/`)
3. Redistributable dependencies (in `redistributable/`)
4. Test dependencies (in `tests/lib`)
4. Test data (in `tests/data`)

These sets of submodules can be cloned additively/independent of eachother.
For most use-cases (like e.g. most of the Docker examples) you should clone the submodules for external dependencies:
```
cd BackpropTools
```
```
git submodule update --init --recursive -- external
```

The submodules for the embedded platforms, the redistributable binaries and test dependencies/data can be cloned in the same fashion (by replacing `external` with the appropriate folder from the enumeration above). 
Note: Make sure that for the redistributable dependencies and test data `git-lfs` is installed (e.g. `sudo apt install git-lfs` on Ubuntu) and activated (`git lfs install`) otherwise only the metadata of the blobs is downloaded.

### Docker
The most deterministic way to get started using <span style="color:#7DB9B6">BackpropTools</span> not only for replication of the results but for modifying the code is using Docker. In our experiments on Linux using the NVIDIA container runtime we were able to achieve close to native performance.
[Docker instructions & examples](examples/docker/README.MD)

### Native
In comparison to running the release binaries or building from source in Docker, the native setup heavily depends on the configuration of the machine it is run on (installed packages, overwritten defaults etc.). Hence we provide guidelines on how to setup the environment for research and development of <span style="color:#7DB9B6">BackpropTools</span> that should run on the default configuration of the particular platform but might not work out of the box if it has been customized.  
#### Unix (Linux and macOS)
For maximum performance and malleability for research and development we recommend to run <span style="color:#7DB9B6">BackpropTools</span> natively on e.g. Linux or macOS. Since <span style="color:#7DB9B6">BackpropTools</span> itself is dependency free the most basic examples don't need any platform setup. However, for an improved experience, we support HDF5 checkpointing and Tensorboard logging as well as optimized BLAS libraries which comes with some system-dependent requirements. 
[Unix instructions & examples](examples/unix/README.MD)

#### Windows
[Windows instructions & examples](examples/windows/README.MD)

## Embedded Platforms
### Inference & Training
- [iOS](https://github.com/BackpropTools/iOS)
- [teensy](https://github.com/BackpropTools/teensy)
### Inference
- [CrazyFlie](embedded_platforms/crazyflie)
- [ESP32](https://github.com/BackpropTools/esp32)
- [PX4](https://github.com/BackpropTools/backprop_tools_px4)

## Naming Convention
We use `snake_case` for variables/instances, functions as well as namespaces and `PascalCase` for structs/classes. Furthermore, we use upper case `SNAKE_CASE` for compile-time constants. 
