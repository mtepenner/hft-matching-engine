.PHONY: all build clean test bench docker-up docker-down

CXX_DIR  := exchange_core
GW_DIR   := fix_gateway
MDA_DIR  := market_data_api
UI_DIR   := trading_terminal

all: build

build: build-core build-gateway build-market-data build-ui

build-core:
	cmake -B $(CXX_DIR)/build -S $(CXX_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(CXX_DIR)/build -j$$(nproc)

build-gateway:
	cd $(GW_DIR) && go build ./...

build-market-data:
	cd $(MDA_DIR) && go build ./...

build-ui:
	cd $(UI_DIR) && npm install && npm run build

test:
	cd $(GW_DIR) && go test ./...
	cd $(MDA_DIR) && go test ./...

bench:
	$(CXX_DIR)/build/exchange_core 2

load-test:
	python3 scripts/load_tester.py --orders 50000

docker-up:
	docker compose up --build -d

docker-down:
	docker compose down

clean:
	rm -rf $(CXX_DIR)/build
	cd $(UI_DIR) && rm -rf node_modules dist
