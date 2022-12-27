# QTM Low-latency Sonification

This project uses the Bela hardware device to sonify motion capture data from Qualisys Track Manager (QTM).
# Contents

<!--ts-->
* [QTM Low-latency Sonification](#qtm-low-latency-sonification)
* [Contents](#contents)
* [Cognitive Science Bachelor's Project (WORK IN PROGRESS)](#cognitive-science-bachelors-project-work-in-progress)
* [Project Structure](#project-structure)
* [Usage](#usage)
   * [Sonification](#sonification)
   * [Data](#data)
      * [Subject Information](#subject-information)
      * [QTM Data Format](#qtm-data-format)
   * [Data Preparation](#data-preparation)
* [Open Source Libraries](#open-source-libraries)
   * [Included](#included)
   * [Used](#used)
<!--te-->

# Cognitive Science Bachelor's Project (WORK IN PROGRESS)

The thesis was written around the investigation of the effect of low-latency sonification on the synchroniciy of joint action tasks.
The project was supervised by [Anna Zamm](https://pure.au.dk/portal/en/persons/anna-zamm(34046139-7057-4cae-927d-f2458b279026).html) (PhD, Assistant Professor, Aarhus University).

The ~~final~~ WIP thesis can be found in this repository in the `docs` directory in the following formats:

- [PDF](docs/CogSci_Bachelor_Thesis.pdf)
- [LaTeX](docs/CogSci_Bachelor_Thesis.tex)
- [R Markdown](docs/CogSci_Bachelor_Thesis.Rmd)

The R Markdown file can be used to rerun the analyses and generate the LaTeX and PDF files.

# Project Structure

The project is structured as follows:

- [`data`](data): Contains the anonymized raw data from the experiment
- [`docs`](docs): Contains the final thesis and other documentation
- [`docs/templates`](docs/templates): Contains the latex template used for the thesis as well as the citation style
- [`res`](res): Contains the resources for the project (audio files, images, etc.)
- [`scripts`](scripts): Scripts used to prepare the data for analysis
- [`src`](src): Contains the source code for the project
- [`src/qsdk`](src/qsdk): Contains the source code for the Qualisys SDK
- [`src/res`](src/res): Soundfiles used in auditory stimuli generation
- [`src/utils`](src/utils): Various functions sort-of organised into what they do (sound, spatial, etc)
- [`src/utils/config.h`](src/utils/config.h): Pretty much anything you would want to change is in here, the experiment, label, and sonification options
- [`src/utils/globals.h`](src/utils/globals.h): Global variables and constants. I think defining things here (especially instead of in the main render loop) can help bela performance to avoid mallocs?
- [`src/utils/latency_check.h`](src/utils/latency_check.h): If this file is included, you can get the output of a round-trip QTM API call, saved in `/var/log/qtm_latency.log` on the Bela.
- [`src/render.cpp`](src/render.cpp): The main Bela sonification application
- [`src/settings.json`](src/settings.json): The Bela settings file that is used by default


# Usage

## Sonification

1. Connect the Bela to the computer where Qualisys Track Manager (or with network access) will run via USB.
2. Clone / download the repository to your PC **important note:** if you would like to access the data in the `data` directory, you will need to install [Git LFS](https://git-lfs.github.com/) and run `git lfs pull` after cloning.
    ```sh
    git clone https://github.com/zeyus/QTM_Bela_Sonification.git
    # optionally, with git lfs
    git lfs pull
    ```
3. Start Qualisys Track Manager, open a project with at least 2 labelled markers
4. Edit `src/render.cpp` to change the marker names to match the ones in your project and the IP address of the computer running QTM
5. TBD: sonification scheme
6. Copy the src directory to the Bela project directory
    ```sh
    scp -r QTM_Bela_Sonification/src root@192.168.2.6:/root/Bela/projects/QTM_Bela_Sonification
    ```
7. Start the QTM recording / playback with real-time output enabled
8. Connect to the Bela board via SSH and run the project
    ```sh
    ssh root@192.168.2.6
    /root/Bela/scripts/run_project.sh QTM_Bela_Sonification -c "--use-analog no --use-digital no --period 32 --high-performance-mode --stop-button-pin=-1 --disable-led"
    ```

**Important note: When compiling, you must ensure that the compiler is in C++14 mode by using `CPPFLAGS=-std=c++14`**

## Data

### Subject Information

Only relevant if you have collected your own data, using the same setup as the thesis.

Will be read from the `data/raw` directory, and the subject information will be used to generate the plots and condition information.

It should be called `subject_information.tsv` and have the following required columns:

- id \[string\]: The subject ID
- trial_order \[n|t|y\]: n = no sonification, t = task based, y = sync based
- partner \[string\]: The partner ID
- door_or_window \[d|w\]: The door or window side (room specific, a way of distinguishing which of the two tracks a subject used)
- handedness \[l|r|a|NA\]: The handedness of the subject (left, right, ambidextrous, or no answer)
- age \[range n|NA\]: The age range bracket the subject reported, or no answer
- tone_deaf \[y|n|NA\]: Whether the subject reported being tone deaf, or no answer
- gender \[string|NA\]: The gender reported by the subject, or no answer
- years_formal_music_training \[float|NA\]: The number of years of formal music training reported by the subject, or no answer
- rating1
- rating2
- rating3
- rating4


### QTM Data Format

The raw data are expected to be 3D data exported from QTM as a TSV including the events/labels. The files should be placed in the `data/raw` directory and should be named as follows:

```
qtm_capture_{subjecta}_{subjectb}{_suffix}.tsv
```

Where:

- `subjecta` is one of the subject ids (e.g. `azsw`) (can be anything valid in a filename but will stop at the first `_` or `.`)
- `subjectb` is the other subject id (e.g. `wsza`) (can be anything valid in a filename but will stop at the first `_` or `.`)
- `_suffix` is an optional suffix (e.g. `_test`) (can be anything valid in a filename but will stop at the file extension `.tsv`)

Example (using above):

```
qtm_capture_azsw_wsza_test.tsv
```


## Data Preparation

The data preparation can be replicated by running the R scripts in the [`scripts`](scripts) directory.

- [`scripts/step_01_import_raw_data.R`](scripts/step_01_import_raw_data.R): Imports the raw data from the `data/raw` directory and saves the data (compressed) in the [`data/`](data) directory. **Note: this is only necessary if you are using your own data**
- [`scripts/step_02_combine_data.R`](scripts/step_02_combine_data.R): Combines the data from all of the subjects into a single data frame and saves it in the [`data/combined_data.tsv.bz2`](data/combined_data.tsv.bz2) file
- [`scripts/step_03_remove_invalid_trials.R`](scripts/step_03_remove_invalid_trials.R): Removes trials that are invalid (e.g. due to missing data) and saves the data in the [`data/combined_data_labeled.tsv.bz2`](data/combined_data_labeled.tsv.bz2) file
  - Invalid trials are removed
  - Trial sequence numbers are added
  - Condition labels are added
  - Only data within trials are retained
- [`scripts/step_04_align3D.R`](scripts/step_04_align3D.R): WIP: Aligns the 3D data to the track reference markers based on known dimensions and relative offsets


# Open Source Libraries

## Included

- [Qualisys C++ SDK](https://github.com/qualisys/qualisys_cpp_sdk) ([MIT License](https://github.com/qualisys/qualisys_cpp_sdk/blob/master/LICENSE.md))

## Used

- [Bela](https://github.com/BelaPlatform/Bela) ([LGPL 3.0 License](https://github.com/BelaPlatform/Bela/blob/master/LICENSE))
- [math-neon](https://code.google.com/archive/p/math-neon/) ([MIT License](https://code.google.com/archive/p/math-neon/))
