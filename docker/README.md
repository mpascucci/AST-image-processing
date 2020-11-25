# ASTimp docker instructions

# create the docker image
`cd` to the docker docker folder and run this command to build the docker image:
```sh
sudo docker build -t astimp .
```
This will create a docker image named "astimp" and setup everything.
The ASTimp sources will be in `/home/AST-image-processing/`

# run the docker image

Use one of these commands depending on what you want to do:

* simply run the docker image
```sh
sudo docker run -it astimp
```

* run the C++ example program
```sh
xhost +local:root
sudo docker run -it --env="DISPLAY" --env="QT_X11_NO_MITSHM=1" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" astimp \
            /bin/bash -c "cd /home/AST-image-processing/build/tests/ && ./fullExample test0.jpg"
xhost -local:root
```

* run jupyter-lab in the directory of the example notebook
```sh
sudo docker run -it -p 8888:8888 astimp \
    jupyter-lab "/home/AST-image-processing/python-module/jupyter-lab-example/" \
    --port=8888 --ip=0.0.0.0 --allow-root --no-browser
```
then open in a browser (or `Ctrl` + click on) the link shown in the terminal window.
