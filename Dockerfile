FROM alpine:latest as builder

RUN apk update && apk add --no-cache gcc libsodium-dev libsodium libc-dev make

WORKDIR /app

COPY . .

RUN make -j$(nproc)

FROM alpine:latest as runner

RUN apk update && apk add --no-cache libsodium

WORKDIR /app

COPY --from=builder /app/a.out .

ENTRYPOINT ["/app/a.out"]
