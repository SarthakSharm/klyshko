# Dockerfile scaffolded by the Operator SDK.

# Build the manager binary
FROM golang:1.16 as builder

ARG RELEASE_PAGE="https://github.com/carbynestack/klyshko/releases"

WORKDIR /workspace
# Copy go.mod and go.sum to leverage Docker cache.
COPY go.mod go.sum ./
RUN go mod download

# Copy the full source tree.
COPY . .

# Ensure the 'bin' folder exists so that controller-gen and other tools are placed there.
RUN mkdir -p bin

# Download the controller-gen binary.
RUN make controller-gen

# Generate deep copy methods and CRD manifests.
RUN make generate manifests

# Build the operator binary.
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -a -o manager main.go

# Use compliant base image from carbynestack/base-images
FROM ghcr.io/carbynestack/ubuntu:20.04-20210827-nonroot

# Copy resources
COPY --from=builder /workspace/manager /
COPY 3RD-PARTY-LICENSES /3RD-PARTY-LICENSES

USER root:root
RUN chmod -R ugo+rXw /3RD-PARTY-LICENSES/disclosure.md
RUN printf "\n## Klyshko Operator\n\
General information about third-party software components and their licenses, \
which are distributed with Klyshko Operator, can be found in the \
[SBOM](./sbom.json). Further details are available in the subfolder for the \
respective component or can be downloaded from the \
[Klyshko Release Page](%s).\n" "${RELEASE_PAGE}"\
  >> /3RD-PARTY-LICENSES/disclosure.md

#  Group and user IDs are defined in base image (non-numerical values, i.e., cs:cs, are not working in k8s)
USER 1000:1000

ENTRYPOINT ["/manager"]
