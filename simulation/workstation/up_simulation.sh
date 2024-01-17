#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

if [[ "$0" = "$BASH_SOURCE" ]]; then
    SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
else # when source me
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

if [ -n "${TELAF_IN_CONTAINER}" ]; then # [Docker-Container-Env]

    # Once again to ensure the 'ssh-server' to be accessed normally
    export TELAF_IN_CONTAINER=yes

    export PATH=/legato/systems/current/bin:$PATH
    export PATH=/venv/bin:$PATH

    source $HOME/simulation/framework/environ.sh

    export TELAF_OK=0
    export TELAF_ERR=1
    export TELAF_TRUE=1
    export TELAF_FALSE=0

    function change_PS1()
    {
        if [ -e "$HOME/.whoami" ]; then
            # prompt on master: [TelAF Simulation] (master):/path/to/here #
            # prompt on slavex: [TelAF Simulation] (slave@ip-address):/path/to/here #
            # Every time you login the container, the prompt should change accordingly
            export CONTAINER_WHO_AM_I=$(awk '{print $1}' $HOME/.whoami)
            if [ "$CONTAINER_WHO_AM_I" == "master" ];then
                export PS1='[\[\e[0;33m\]TelAF Simulation\[\e[0m\]] (\[\e[1;31m\]\[\e[1m\]$CONTAINER_WHO_AM_I\[\e[0m\]):\w \$ '
            else
                SLAVE_IP=$(awk '{print $2}' $HOME/.whoami)
                export PS1='[\[\e[0;33m\]TelAF Simulation\[\e[0m\]] (\[\e[1;31m\]\[\e[1m\]$CONTAINER_WHO_AM_I@$SLAVE_IP\[\e[0m\]):\w \$ '
            fi
        fi
    }

    change_PS1

    if [ "$CONTAINER_WHO_AM_I" == "master" ];then
        echo
        echo "Welcome to TelAF Simulation Environment, enter 'telaf start' to launch!"
        echo "# Caution: If you want to keep the DATA persistently, please put them into '/root/simulation' directory or volumes!"
        echo
    fi

    # Add a hook script for doing some thing every time you login
    if [ -f $HOME/simulation/.simula.always.sh ]; then
        source $HOME/simulation/.simula.always.sh
    fi

    if [ -f /tmp/telaf_simulation_up ]; then
        return # multi-user access with ssh-tool ? keep ONLY once init-action.
    fi

    # Prepare some ssh client/server options in advance
    sed -i 's/^#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
    sed -i 's/^#PrintLastLog.*/PrintLastLog no/' /etc/ssh/sshd_config
    # sed -i 's/^#LogLevel.*/LogLevel quiet/' /etc/ssh/sshd_config
    sed -i 's/^#   StrictHostKeyChecking.*/    StrictHostKeyChecking no/' /etc/ssh/ssh_config
    sed -i '/motd.dynamic/ s/^/#/' /etc/pam.d/sshd
    service ssh start >/dev/null 2>&1

    # Stop displaying "warranties" notice when logging in by ssh-client
    mkdir -p $HOME/.cache
    touch $HOME/.cache/motd.legal-displayed

    cnode_ipv4=$(hostname -i | grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b")
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        echo "$CONTAINER_NAME $cnode_ipv4" > $HOME/simulation/.simula.master
        # $HOME/.whoami to indicate the container node identity: master or slave
        echo "$CONTAINER_WHO_AM_I $cnode_ipv4 $CONTAINER_NAME" > $HOME/.whoami
    elif [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        echo "$CONTAINER_WHO_AM_I $cnode_ipv4 $CONTAINER_NAME" > $HOME/.whoami
    fi

    # Create SSH key for access between different containers [master -> slavex]
    M_SSH_HOME=$HOME/simulation/.ssh/${CONTAINER_WHO_AM_I}

    # The key is updated every time, we delete and then rebuild.
    if [ -e "$M_SSH_HOME" ];then
        rm -rf $M_SSH_HOME
    fi

    # Create a new ssh key to authenticate
    # Ensure that the authentication file has the correct permissions.
    mkdir -m 700 -p $M_SSH_HOME
    ssh-keygen -t rsa -b 2048 -N "" -f "$M_SSH_HOME/id_rsa" > /dev/null 2>&1
    touch $M_SSH_HOME/authorized_keys
    chmod 600 $M_SSH_HOME/authorized_keys

    # Register 'master' pub key to 'slave-x'
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        if [ -f "$HOME/simulation/.slavex" ]; then
            cat $HOME/simulation/.ssh/master/id_rsa.pub >> $HOME/simulation/.ssh/slave/authorized_keys
        fi
    fi

    # Redirect ssh key location, without 'umount' at the end, just 'exit' is fine.
    mkdir -m 700 -p $HOME/.ssh
    mount --bind $M_SSH_HOME $HOME/.ssh

    # Create some default groups
    groupadd system
    groupadd diag
    groupadd radio
    groupadd inet
    groupadd locclient

    # Create some default users
    useradd -M --no-log-init --shell /bin/bash telaf
    useradd -M --no-log-init --shell /bin/bash appdefault

    echo "/mnt/legato/system/lib" > /tmp/ld.so.conf

    # Remap 'reboot' to a special script, don't restart the container
    unlink /sbin/reboot
    echo "/usr/bin/killall serviceDirectory" > /tmp/simulation_reboot

    # When the framework starts, if the supervisor encounters a FATAL error,
    # we need to make sure that the docker container does not exit
    echo "/usr/bin/killall startSystem"     >> /tmp/simulation_reboot

    chmod a+x /tmp/simulation_reboot
    ln -s /tmp/simulation_reboot /sbin/reboot

    # Enable read-only mode in the container environment
    cp /etc/passwd /tmp/passwd
    cp /etc/group /tmp/group
    mount --bind -o ro /tmp/passwd /etc/passwd
    mount --bind -o ro /tmp/group  /etc/group

    # Change the hostname to a specific label: simulation
    # Also be used for syslog tag
    hostname simulation

    # Busybox syslogd on Ubuntu
    /sbin/syslogd -C20000

    if [ -f /usr/bin/logread ]; then
        ln -s /usr/bin/logread /sbin/logread
    else
        ln -s /bin/logread /sbin/logread
    fi

    mkdir -p /legato
    mkdir -p /tmp/legato_logs
    mkdir -p /tmp/legato

    mkdir -p /data/le_fs
    mkdir -p /data/persist
    mkdir -p /data/ManagedServices
    chmod 0777 /data/le_fs

    MOUNTPOINT_TELAF="/mnt/legato"
    mount -o bind $MOUNTPOINT_TELAF /legato

    # Use SIMULATION_TARBALL_NAME to ensure which image we could use
    SML_RO_TARBALL=$HOME/simulation/$SIMULATION_TARBALL_NAME

    if [ -f $SML_RO_TARBALL ]; then

        # extract the tarball to /mnt/legato without the 'install/' directory
        tar zxf $SML_RO_TARBALL --no-same-owner --overwrite -C $MOUNTPOINT_TELAF --exclude up_simulation.sh

        if tar tzvf $SML_RO_TARBALL | grep 'taf_rootfs/.keep' > /dev/null 2>&1 ; then
            # extract the 'taf_rootfs/' directory to /usr/lib only, cut down 2-level parent-dirs
            tar zxf $SML_RO_TARBALL --no-same-owner --overwrite --strip-components=2 -C /usr/lib taf_rootfs
        else
            echo "[taf_rootfs/.keep] isn't in [$SML_RO_TARBALL], please check it first, stop simulation."
            exit 1
        fi

        SDK_ROOTFS=/legato/systems/current/sdk_rootfs
        if [ -d ${SDK_ROOTFS} ]; then
            # Follow SDK Dockerfile configuration
            cp -a -r -d ${SDK_ROOTFS}/bin/* /usr/bin/
            cp -a -r -d ${SDK_ROOTFS}/lib/* /usr/lib/
            cp -a -r -d ${SDK_ROOTFS}/include/* /usr/include/
            cp -a -r -d ${SDK_ROOTFS}/share/* /usr/share/
            cp -a -r -d ${SDK_ROOTFS}/etc/* /etc/
            cp -a -r -d ${SDK_ROOTFS}/data/* /data/

            # Change the dirs' mode for others access
            chmod 0766 /etc/telux/*
            chmod -R 0777 /data/telux

            # Use supervisord to start and monitor telsdk_simulation_server
            supervisord -c /etc/supervisord.conf
        fi

        chmod 755 $MOUNTPOINT_TELAF/systems/current/bin/*

        from_version=`cat $MOUNTPOINT_TELAF/.check_done`

        if [ -e $MOUNTPOINT_TELAF/.check_done ] ; then
            if [ "$from_version" != "from 18.04" ] && [ "$from_version" != "from 20.04" ]; then
                echo "Exist .check_done, but [$from_version], not in [18.04, 20.04], please rebuild your tarball."
                exit 1
            fi

            # Script that is only used for initialization once
            if [ -f $HOME/simulation/.simula.once.sh ]; then
                source $HOME/simulation/.simula.once.sh
            fi

            # here we go, happy to simulate

        else
            echo "Tarball NOT contains .check_done, this is a bad ball, please rebuild right one."
            exit 1
        fi

    else
        echo "Not found $SML_RO_TARBALL, please get it first !"
        exit 0
    fi

    # Mark done
    touch /tmp/telaf_simulation_up
    change_PS1

else # [Non-Docker-Container-Env]

    trap : INT TERM

    if [ $# -ge 1 ]; then
        export CONTAINER_WHO_AM_I="$1"
    else
        export CONTAINER_WHO_AM_I="master"
    fi

    function try_to_create_volume () {
        if docker volume inspect $1 > /dev/null 2>&1; then
            echo "[Volume Reuse] --> $1"
        else
            echo "[Volume Create] --> $1"
            docker volume create $1 > /dev/null
        fi
    }

    SML_WORKSPACE=$SCRIPT_DIR

    IMG_NAME=${IMG_NAME:="telaf_simulation_runtime"}
    IMAGE_EXISTS=$(docker images --filter "reference=$IMG_NAME" | grep $IMG_NAME | wc -l)

    if [ ! $IMAGE_EXISTS -gt 0 ]; then
        echo "> Docker image '$IMG_NAME' NOT exists, please build or pull this image firstly !"
        exit 1
    fi

    if [ ! -f $SML_WORKSPACE/telaf_simulation.tar.gz ]; then
        echo "> Put 'up_simulation.sh' and 'telaf_simulation.tar.gz' to the SAME directory !"
        exit 1
    fi

    # The priority of variables is as follows:
    #  1. simulation.cfg (if exists)
    #  2. Environmental parameters, such as: 'CONTAINER_NAME=telaf_sml_alias ./up_simulation.sh'
    #  3. Default values in this script

    if [ -f $SML_WORKSPACE/simulation.cfg ]; then
        source $SML_WORKSPACE/simulation.cfg
    fi

    CONTAINER_NAME=${CONTAINER_NAME:="telaf_simulation_runtime"}
    IMG_NAME=${IMG_NAME:="telaf_simulation_runtime"}
    IMG_VERSION=${IMG_VERSION:="1.0.0"}
    IPV6_NETWORK_NAME=${IPV6_NETWORK_NAME:="${CONTAINER_NAME%??}_ipv6net"}
    IPV6_DEFAULT_SUBNET=${IPV6_DEFAULT_SUBNET:="2001:0DB8::/112"}
    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="-i -t --privileged=true --cgroupns=private --net=$IPV6_NETWORK_NAME"}
    CONTAINER_OPTIONS=${CONTAINER_OPTIONS:="-p 9022:22 --rm"}
    SIMULATION_TARBALL_NAME=${SIMULATION_TARBALL_NAME:="telaf_simulation.tar.gz"}

    SML_APP_VOLUME=${CONTAINER_NAME}_sml_app
    SML_DATA_VOLUME=${CONTAINER_NAME}_sml_data
    SML_PERSIST_VOLUME=${CONTAINER_NAME}_sml_persist
    SML_MNT_LEGATO_VOLUME=${CONTAINER_NAME}_sml_mnt_legato

    echo "[Prepare] Create or Reuse docker volumes for persistently data"
    try_to_create_volume $SML_APP_VOLUME
    try_to_create_volume $SML_DATA_VOLUME
    try_to_create_volume $SML_PERSIST_VOLUME
    try_to_create_volume $SML_MNT_LEGATO_VOLUME

    networks=$(docker network ls --format "{{.Name}}")

    if echo "$networks" | grep -w "$IPV6_NETWORK_NAME" &>/dev/null; then
        echo "[Network Reuse] --> $IPV6_NETWORK_NAME"
    else
        echo "[Network Create] --> $IPV6_NETWORK_NAME"
        docker network create --driver bridge --ipv6 --subnet "$IPV6_DEFAULT_SUBNET" "$IPV6_NETWORK_NAME" > /dev/null
        if [ $? -ne 0 ]; then
            echo "There is a conficting network [address: $IPV6_DEFAULT_SUBNET is reused], check docker network and delete that for continuing"
            exit 1
        fi
    fi

    CMD="docker run --name $CONTAINER_NAME \
        $BUILTIN_CONTAINER_OPTIONS \
        $CONTAINER_OPTIONS \
        -e CONTAINER_WHO_AM_I=$CONTAINER_WHO_AM_I \
        -e CONTAINER_NAME=$CONTAINER_NAME \
        -e SIMULATION_TARBALL_NAME=$SIMULATION_TARBALL_NAME \
        -v $SML_WORKSPACE:/root/simulation:rw \
        -v $SML_APP_VOLUME:/app:rw \
        -v $SML_DATA_VOLUME:/data:rw \
        -v $SML_PERSIST_VOLUME:/persist:rw \
        -v $SML_MNT_LEGATO_VOLUME:/mnt/legato:rw \
        $IMG_NAME:$IMG_VERSION /bin/bash"
    echo
    echo "> "$CMD
    eval $CMD

    if [ $? -ne 0 ]; then
        exit $?
    fi

    # mark slave-x to be started, also store the IP address related.
    if [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$CONTAINER_NAME")
        echo -e "\n> Slave daemon [${CONTAINER_NAME}] is started @ [$CONTAINER_IP].\n"
        echo "$CONTAINER_NAME $CONTAINER_IP" >> $SML_WORKSPACE/.slavex
        echo "$CONTAINER_NAME $CONTAINER_IP" > $SML_WORKSPACE/.simula.slave
    fi

    # master record the container names & IP addresses from slave-x.
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        if [ -f $SML_WORKSPACE/.slavex ]; then
            while IFS=' ' read -r cname ipaddr;
            do
                echo "> Stop [$cname] partner @ [$ipaddr] ..."
                docker stop $cname > /dev/null 2>&1
                echo "> Stop [$cname] partner @ [$ipaddr] done."
            done < "$SML_WORKSPACE/.slavex"

            rm -f $SML_WORKSPACE/.slavex $SML_WORKSPACE/.simula.slave $SML_WORKSPACE/.simula.master
            exit $?
        else
            # no partner ? ok, exit directly.
            exit 0
        fi
    elif [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        exit $?
    else # standalone ?
        exit 0
    fi
fi
