FROM ubuntu:20.04

ENV TZ=Europe/Paris
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update

# Essential packages
RUN apt-get install -y apt-utils build-essential unzip git

# ASTimp dependencies
RUN apt-get install -y libopencv-dev cmake libgtest-dev

# clone astimp and compile it
RUN cd home
RUN git clone https://github.com/mpascucci/AST-image-processing.git /home/AST-image-processing
RUN ./home/AST-image-processing/run-build.sh

# Install Python wrapper module and Juputer
RUN apt-get install -y python3-pip python3-opencv
RUN pip3 install jupyterlab
RUN cd /home/AST-image-processing/python-module/ && ./install_astimp_python_module.sh
RUN echo LD_LIBRARY_PATH=/home/AST-image-processing/build/astimplib:$LD_LIBRARY_PATH >> /root/.bashrc
RUN unzip "/home/AST-image-processing/python-module/jupyter-lab-example/ASTimp Quick Start with python.zip"
