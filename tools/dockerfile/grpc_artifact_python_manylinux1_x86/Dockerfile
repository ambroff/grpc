# Copyright 2016 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Docker file for building gRPC manylinux Python artifacts.

FROM quay.io/pypa/manylinux1_i686

# Update the package manager
RUN yum update -y
RUN yum install -y curl-devel expat-devel gettext-devel linux-headers openssl-devel zlib-devel gcc

###################################
# Install Python build requirements
RUN /opt/python/cp27-cp27m/bin/pip install cython
RUN /opt/python/cp27-cp27mu/bin/pip install cython
RUN /opt/python/cp34-cp34m/bin/pip install cython
RUN /opt/python/cp35-cp35m/bin/pip install cython
RUN /opt/python/cp36-cp36m/bin/pip install cython
RUN /opt/python/cp37-cp37m/bin/pip install cython
RUN /opt/python/cp37-cp37m/bin/pip install cython
RUN /opt/python/cp38-cp38/bin/pip install cython
