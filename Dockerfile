## Compile executable
FROM alpine:3 AS compiler

# Install required packages
RUN apk add --no-cache cmake gcc make bash musl-dev

# Set the working directory inside the container
WORKDIR /app/UsagiInit

# Copy the entire project directory into the container
COPY . .

# Make sure the script is executable
RUN chmod +x scripts/release_static.sh

# Run the build script
RUN ./scripts/release_static.sh

## Compress executable
FROM debian:12 AS compressor

WORKDIR /tmp

COPY --from=0 /app/UsagiInit/UsagiInit /UsagiInit

# Install dependencies
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential curl ca-certificates

ARG UPX_VERSION=5.0.2

RUN curl -L https://github.com/upx/upx/releases/download/v${UPX_VERSION}/upx-${UPX_VERSION}-amd64_linux.tar.xz -o /tmp/upx.tar.xz \
 && tar -xf /tmp/upx.tar.xz -C /tmp \
 && mv /tmp/upx-${UPX_VERSION}-amd64_linux/upx /usr/local/bin/upx

RUN upx --best --lzma /UsagiInit

# Final stage
FROM busybox:stable-musl

COPY --from=compressor /UsagiInit /UsagiInit

# Set default command
ENTRYPOINT ["/UsagiInit"]
