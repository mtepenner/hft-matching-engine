FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y cmake g++ make libboost-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)"

FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/build/exchange_core .
EXPOSE 7700
CMD ["./exchange_core"]
