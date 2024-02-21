ARG IMAGE=ubuntu:focal
FROM ${IMAGE}

RUN apt-get update && apt-get install -y --no-install-recommends sudo git bash-completion && rm -rf /var/lib/apt/lists/*

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid ${USER_GID} ${USERNAME} && \
    useradd -s /bin/bash --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} && \
    echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && \
    chmod 0440 /etc/sudoers.d/${USERNAME}

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cmake ninja-build g++ libxft-dev libxcursor-dev && \
	rm -rf /var/lib/apt/lists/*
