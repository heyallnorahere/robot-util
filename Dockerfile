ARG IMAGE=debian:bookworm
FROM --platform=$BUILDPLATFORM ${IMAGE} AS build
SHELL [ "/bin/bash", "-c" ]

WORKDIR /robot-util
COPY scripts scripts

ARG TARGETOS
ARG TARGETARCH
ARG BUILDARCH

RUN scripts/buildenv.sh ${TARGETOS} ${TARGETARCH} ${BUILDARCH}

COPY CMakeLists.txt .
COPY vendor vendor
COPY src src

RUN scripts/build.sh ${TARGETOS} ${TARGETARCH}

FROM ${IMAGE} AS runtime
SHELL [ "/bin/bash", "-c" ]
WORKDIR /robot-util

COPY scripts scripts
RUN scripts/runtimeenv.sh

COPY --from=build /robot-util/build ./build

LABEL org.opencontainers.image.source=https://github.com/heyallnorahere/robot-util
LABEL org.opencontainers.image.licenses=Apache-2.0

ENTRYPOINT [ "/robot-util/build/src/robot-util" ]
