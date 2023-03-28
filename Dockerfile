FROM ubuntu:trusty

RUN mkdir installing \
    && cd installing \
    && apt-get update \
    && apt-get install curl gcc-avr avr-libc lib32ncurses5 cpio build-essential make --yes \
    && curl -OL https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q1-update/+download/gcc-arm-none-eabi-4_8-2014q1-20140314-linux.tar.bz2

WORKDIR installing

RUN tar xvjf gcc-arm-none-eabi-4_8-2014q1-20140314-linux.tar.bz2 \
    && mv gcc-arm-none-eabi-4_8-2014q1 /opt/ARM \
    && rm gcc-arm-none-eabi-4_8-2014q1-20140314-linux.tar.bz2

ENV PATH="/opt/ARM/bin:${PATH}"

WORKDIR /app
