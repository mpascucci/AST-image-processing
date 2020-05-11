# How to update the TensorFlow sources and libraries

- Download the latest tensorflow-lite-0.0.0-nightly.aar from [JFrog](https://bintray.com/google/tensorflow/tensorflow-lite/0.0.0-nightly#files/org%2Ftensorflow%2Ftensorflow-lite%2F0.0.0-nightly). 

- Rename the .aar to .zip and unpack.

- Update Android shared libraries:

    - `cp -r tensorflow-lite-0.0.0-nightly/jni/* astapp/improc/astimplib/vendor/tensorflow/lib`

- Update TensorFlow Lite headers:

    - `cp -r tensorflow-lite-0.0.0-nightly/headers/tensorflow astapp/improc/astimplib/vendor` 

## Building for Mac OS & Linux

- Pull the latest Tensorflow from [github repo](https://github.com/tensorflow/tensorflow).

- Build the new Mac OS and Linux libraries using those respective systems and Bazel:

    - `bazel build //tensorflow/lite/c:tensorflowlite_c -c opt`

Follow these instructions to setup Bazel: https://www.tensorflow.org/install/source

The full set of commands on my MacOS was:
```
# Install HomeBrew
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
export PATH="/usr/local/bin:/usr/local/sbin:$PATH"
cd ~/gits

# Create virtual environment for Python
python3 -m venv tflite
cd tflite/
source bin/activate
pip install -U pip six numpy wheel setuptools mock 'future>=0.17.1'
pip install -U keras_applications --no-deps
pip install -U keras_preprocessing --no-deps

# Install Bazelisk
cd ~/gits
git clone https://github.com/bazelbuild/bazelisk.git
brew install golang
go get github.com/bazelbuild/bazelisk
export PATH=$PATH:$(go env GOPATH)/bin
./build.sh 
ln -sf `which bazelisk` /usr/local/bin/bazel

cd ~/gits/tensorflow
git pull
./configure 
bazel build //tensorflow/lite/c:tensorflowlite_c -c opt
cp ~/gits/tensorflow/bazel-bin/tensorflow/lite/c/libtensorflowlite_c.dylib ~/gits/astapp/improc/astimplib/vendor/tensorflow/lib/osx_x86_64
```