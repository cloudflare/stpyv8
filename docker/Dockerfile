ARG ARG_PYTHON_VERSION=3.7
FROM python:$ARG_PYTHON_VERSION-buster

ARG ARG_PYTHON_VERSION=3.7
ARG DEBIAN_FRONTEND=noninteractive

ADD libstdc++.so.6.0.28 /usr/lib/x86_64-linux-gnu/
ADD libstdc++.a /usr/lib/x86_64-linux-gnu/

RUN set -x && \
    rm /usr/lib/x86_64-linux-gnu/libstdc++.so.6 && \
    ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.28 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 && \
    rm /usr/lib/gcc/x86_64-linux-gnu/8/libstdc++.a && \
    ln -s /usr/lib/x86_64-linux-gnu/libstdc++.a /usr/lib/gcc/x86_64-linux-gnu/8/libstdc++.a && \
    ln -s /usr/local/include/python${ARG_PYTHON_VERSION}m /usr/local/include/python${ARG_PYTHON_VERSION} && \
    apt update && \
    apt install -y curl git lsb-release vim pkg-config build-essential && \
    python -m pip install virtualenv && \
    git config --global --add safe.directory '*'
