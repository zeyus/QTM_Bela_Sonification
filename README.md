# QTM Low-latency Sonification

This project uses the Bela hardware device to sonify motion capture data from Qualisys Track Manager (QTM).
# Contents

<!--ts-->
* [QTM Low-latency Sonification](#qtm-low-latency-sonification)
* [Contents](#contents)
* [Cognitive Science Bachelor's Project](#cognitive-science-bachelors-project)
* [Project Structure](#project-structure)
* [Instructions](#instructions)
* [Open Source Libraries](#open-source-libraries)
   * [Included](#included)
   * [Used](#used)
<!--te-->

# Cognitive Science Bachelor's Project

The thesis was written around the investigation of the effect of low-latency sonification on the synchroniciy of joint action tasks.
The project was supervised by [Anna Zamm](https://pure.au.dk/portal/en/persons/anna-zamm(34046139-7057-4cae-927d-f2458b279026).html) (PhD, Assistant Professor, Aarhus University).

The final thesis can be found in this repository in the `docs` directory in the following formats:

- [PDF](docs/CogSci_Bachelor_Thesis.pdf)
- [LaTeX](docs/CogSci_Bachelor_Thesis.tex)
- [R Markdown](docs/CogSci_Bachelor_Thesis.Rmd)

The R Markdown file can be used to rerun the analyses and generate the LaTeX and PDF files.

# Project Structure

The project is structured as follows:

- `data`: Contains the anonymized raw data from the experiment.
- `docs`: Contains the final thesis and other documentation
- `docs/templates`: Contains the latex template used for the thesis as well as the citation style
- `res`: Contains the resources for the project (audio files, images, etc.)
- `src`: Contains the source code for the project
- `src/settings.json`: The Bela settings file that is used by default.
- `src/lib/qsdk`: Contains the source code for the Qualisys SDK


# Instructions

1. Connect the Bela to the computer where Qualisys Track Manager (or with network access) will run via USB.
1. Clone / download the repository to your PC
    ```sh
    git clone https://github.com/zeyus/QTM_Bela_Sonification.git
    ```
1. Start Qualisys Track Manager, open a project with at least 2 labelled markers
1. Edit `src/render.cpp` to change the marker names to match the ones in your project and the IP address of the computer running QTM
1. TBD: sonification scheme
1. Copy the src directory to the Bela project directory
    ```sh
    scp -r QTM_Bela_Sonification/src root@192.168.2.6:/root/Bela/projects/QTM_Bela_Sonification
    ```
1. Start the QTM recording / playback with real-time output enabled
1. Connect to the Bela board via SSH and run the project
    ```sh
    ssh root@192.168.2.6
    /root/Bela/scripts/run_project.sh QTM_Bela_Sonification -c "--use-analog yes --use-digital no --period 32 --high-performance-mode"
    ```

# Open Source Libraries

## Included

- [Qualisys C++ SDK](https://github.com/qualisys/qualisys_cpp_sdk) ([MIT License](https://github.com/qualisys/qualisys_cpp_sdk/blob/master/LICENSE.md))

## Used

- [Bela](https://github.com/BelaPlatform/Bela) ([LGPL 3.0 License](https://github.com/BelaPlatform/Bela/blob/master/LICENSE))
- [math-neon](https://code.google.com/archive/p/math-neon/) ([MIT License](https://code.google.com/archive/p/math-neon/))
