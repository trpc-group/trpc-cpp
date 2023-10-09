# gRPC streaming RPC calling.

- Streaming server: An example that transforms the server from
  the [gRPC framework server example](https://github.com/grpc/grpc/blob/master/examples/cpp/route_guide/route_guide_server.cc)
  into an example using the tRPC framework.
- Test
  client: [gRPC framework client example](https://github.com/grpc/grpc/blob/master/examples/cpp/route_guide/route_guide_client.cc).
- *Currently, gRPC streaming client is still in progress.*

Runtime requirements: Please use `Fiber` thread model.

## Usage

We can use the following command to view the directory tree.

```shell
$ tree examples/features/grpc_stream/
examples/features/grpc_stream
├── CMakeLists.txt
├── common
│   ├── BUILD
│   ├── helper.cc
│   ├── helper.h
│   └── route_guide_db.json
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── stream.proto
    ├── stream_server.cc
    ├── stream_service.cc
    ├── stream_service.h
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a server program.

```shell
sh examples/features/trpc_stream/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build //examples/features/grpc_stream/server:grpc_stream_server
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

Building gRPC streaming client.

Install gcc8.
```shell
yum install centos-release-scl
yum install devtoolset-8-gcc devtoolset-8-gcc-c++
```

Enter in gcc8 environment.
```shell
scl enable devtoolset-8 -- bash
```

Compiling gRPC.
```shell
# Clone the grpc repo and its submodules: 
# e.g. git clone --recurse-submodules -b v1.56.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

cd /path-to-grpc-repo/
mkdir /path-to-grpc-repo/.install
export MY_INSTALL_DIR=/path-to-grpc-repo/.install
export PATH="$MY_INSTALL_DIR/bin:$PATH"
mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
make -j
make install
popd
```

Compiling gRPC streaming client.
```shell
cd /path-to-grpc-repo/
cd examples/cpp/route_guide
mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
make -j
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
bazel-bin/examples/features/grpc_stream/server/grpc_stream_server --config=examples/features/grpc_stream/server/trpc_cpp_fiber.yaml --dbpath=examples/features/grpc_stream/common/route_guide_db.json &

```

* Run the client program

We can run the following command to start the client program.

```shell
# Please run `route_guide_client` commands at path `/path-to-grpc-repo/examples/cpp/route_guide` due to it depends on `route_guide_db.json`.
$ pwd 
/path-to-grpc-repo/examples/cpp/route_guide

./cmake/build/route_guide_client
```

The content of the output from the client program is as follows:

``` text

DB parsed, loaded 100 features.
-------------- GetFeature --------------
Found feature called BerkshireValleyManagementAreaTrail,Jefferson,NJ,USA at 40.9146, -74.6189
Found no feature at 0, 0
-------------- ListFeatures --------------
Looking for features between 40, -75 and 42, -73
Found feature called PatriotsPath,Mendham,NJ07945,USA at 40.7838, -74.6144
Found feature called 101NewJersey10,Whippany,NJ07981,USA at 40.8123, -74.3999
Found feature called U.S.6,Shohola,PA18458,USA at 41.3628, -74.9016
Found feature called 5ConnersRoad,Kingston,NY12401,USA at 42, -74.0371
Found feature called MidHudsonPsychiatricCenter,NewHampton,NY10958,USA at 41.4008, -74.3951
Found feature called 287FlugertownRoad,LivingstonManor,NY12758,USA at 41.9611, -74.6525
Found feature called 4001TremleyPointRoad,Linden,NJ07036,USA at 40.611, -74.2187
Found feature called 352SouthMountainRoad,Wallkill,NY12589,USA at 41.6802, -74.237
Found feature called BaileyTurnRoad,Harriman,NY10926,USA at 41.295, -74.1077
Found feature called 193-199WawayandaRoad,Hewitt,NJ07421,USA at 41.2145, -74.395
Found feature called 406-496WardAvenue,PineBush,NY12566,USA at 41.5737, -74.2848
Found feature called 162MerrillRoad,HighlandMills,NY10930,USA at 41.3844, -74.0502
Found feature called ClintonRoad,WestMilford,NJ07480,USA at 41.0873, -74.4459
Found feature called 16OldBrookLane,Warwick,NY10990,USA at 41.2346, -74.4027
Found feature called 3DrakeLane,Pennington,NJ08534,USA at 40.2948, -74.7904
Found feature called 63248thAvenue,Brooklyn,NY11220,USA at 40.6337, -74.0122
Found feature called 1MerckAccessRoad,WhitehouseStation,NJ08889,USA at 40.6422, -74.7728
Found feature called 78-98SchalckRoad,Narrowsburg,NY12764,USA at 41.6318, -74.9678
Found feature called 282LakeviewDriveRoad,HighlandLake,NY12743,USA at 41.5302, -74.8416
Found feature called 330EvelynAvenue,HamiltonTownship,NJ08619,USA at 40.2647, -74.7072
Found feature called NewYorkStateReferenceRoute987E,Southfields,NY10975,USA at 41.2568, -74.1058
Found feature called 103-271TempaloniRoad,Ellenville,NY12428,USA at 41.6855, -74.4421
Found feature called 1300AirportRoad,NorthBrunswickTownship,NJ08902,USA at 40.4664, -74.482
Found feature called  at 40.7114, -74.9746
Found feature called  at 40.2134, -74.3613
Found feature called  at 40.0273, -74.1221
Found feature called  at 41.1237, -74.4071
Found feature called 211-225PlainsRoad,Augusta,NJ07822,USA at 41.1634, -74.6785
Found feature called  at 41.5831, -74.2953
Found feature called 165PedersenRidgeRoad,Milford,PA18337,USA at 41.3447, -74.8713
Found feature called 100-122LocktownRoad,Frenchtown,NJ08825,USA at 40.5047, -74.9801
Found feature called  at 41.8859, -74.6157
Found feature called 650-652WilliHillRoad,SwanLake,NY12783,USA at 41.7952, -74.8485
Found feature called 26East3rdStreet,NewProvidence,NJ07974,USA at 40.7034, -74.3977
Found feature called  at 41.7548, -74.0075
Found feature called  at 41.0396, -74.4972
Found feature called  at 40.4615, -74.513
Found feature called 611LawrenceAvenue,Westfield,NJ07090,USA at 40.659, -74.356
Found feature called 18LannisAvenue,NewWindsor,NY12553,USA at 41.4653, -74.0478
Found feature called 82-104AmherstAvenue,Colonia,NJ07067,USA at 40.5958, -74.3255
Found feature called 170SevenLakesDrive,Sloatsburg,NY10974,USA at 41.1734, -74.1648
Found feature called 1270LakesRoad,Monroe,NY10950,USA at 41.2676, -74.2607
Found feature called 509-535AlphanoRoad,GreatMeadows,NJ07838,USA at 40.9224, -74.8287
Found feature called 652GardenStreet,Elizabeth,NJ07202,USA at 40.6523, -74.2135
Found feature called 349SeaSprayCourt,NeptuneCity,NJ07753,USA at 40.1827, -74.0294
Found feature called 13-17StanleyStreet,WestMilford,NJ07480,USA at 41.0564, -74.3685
Found feature called 47IndustrialAvenue,Teterboro,NJ07608,USA at 40.8472, -74.0726
Found feature called 5WhiteOakLane,StonyPoint,NY10980,USA at 41.2452, -74.0214
Found feature called BerkshireValleyManagementAreaTrail,Jefferson,NJ,USA at 40.9146, -74.6189
Found feature called 1007JerseyAvenue,NewBrunswick,NJ08901,USA at 40.4701, -74.4782
Found feature called 6EastEmeraldIsleDrive,LakeHopatcong,NJ07849,USA at 40.9643, -74.6018
Found feature called 1358-1474NewJersey57,PortMurray,NJ07865,USA at 40.8032, -74.8645
Found feature called 367ProspectRoad,Chester,NY10918,USA at 41.37, -74.2135
Found feature called 10SimonLakeDrive,AtlanticHighlands,NJ07716,USA at 40.4311, -74.0283
Found feature called 11WardStreet,MountArlington,NJ07856,USA at 40.932, -74.6201
Found feature called 300-398JeffersonAvenue,Elizabeth,NJ07201,USA at 40.6685, -74.2109
Found feature called 43DreherRoad,Roscoe,NY12776,USA at 41.9018, -74.9143
Found feature called SwanStreet,PineIsland,NY10969,USA at 41.2856, -74.5149
Found feature called 66PleasantviewAvenue,Monticello,NY12701,USA at 41.6561, -74.6722
Found feature called  at 40.5314, -74.9836
Found feature called  at 41.422, -74.3327
Found feature called 565WindingHillsRoad,Montgomery,NY12549,USA at 41.5534, -74.2901
Found feature called 231RockyRunRoad,GlenGardner,NJ08826,USA at 40.6899, -74.9127
Found feature called 100MountPleasantAvenue,Newark,NJ07104,USA at 40.7587, -74.167
Found feature called 517-521HuntingtonDrive,ManchesterTownship,NJ08759,USA at 40.0106, -74.287
Found feature called  at 40.0066, -74.6793
Found feature called 40MountainRoad,Napanoch,NY12458,USA at 41.8804, -74.4103
Found feature called  at 41.4204, -74.7895
Found feature called  at 41.4777, -74.0616
Found feature called 48NorthRoad,Forestburgh,NY12777,USA at 41.5464, -74.7175
Found feature called  at 40.4062, -74.6376
Found feature called  at 40.5688, -74.9285
Found feature called  at 40.0342, -74.8789
Found feature called  at 40.1809, -74.4158
Found feature called 9ThompsonAvenue,Leonardo,NJ07737,USA at 40.4227, -74.0517
Found feature called  at 41.0322, -74.7872
Found feature called  at 40.7101, -74.7743
Found feature called 213BushRoad,StoneRidge,NY12484,USA at 41.8811, -74.1718
Found feature called  at 41.5034, -74.3851
Found feature called  at 41.135, -74.3694
Found feature called 1-17BergenCourt,NewBrunswick,NJ08901,USA at 40.484, -74.476
Found feature called 35OaklandValleyRoad,Cuddebackville,NY12729,USA at 41.4638, -74.5958
Found feature called  at 41.2128, -74.0174
Found feature called  at 40.1263, -74.7964
Found feature called  at 41.2843, -74.9086
Found feature called  at 41.8513, -74.3068
Found feature called 42-102MainStreet,Belford,NJ07718,USA at 40.4318, -74.0836
Found feature called  at 41.9021, -74.1172
Found feature called  at 40.4081, -74.612
Found feature called  at 40.1013, -74.4035
Found feature called  at 40.4306, -74.108
Found feature called  at 40.3966, -74.8519
Found feature called  at 40.5002, -74.8408
Found feature called  at 40.9533, -74.2201
Found feature called  at 41.6851, -74.2675
Found feature called 3387RichmondTerrace,StatenIsland,NY10303,USA at 40.6412, -74.1722
Found feature called 261VanSickleRoad,Goshen,NY10924,USA at 41.3069, -74.4598
Found feature called  at 41.8465, -74.6859
Found feature called  at 41.1733, -74.4228
Found feature called 3HastaWay,Newton,NJ07860,USA at 41.0248, -74.7128
ListFeatures rpc succeeded.
-------------- RecordRoute --------------
Visiting point 41.2676, -74.2607
Visiting point 41.1734, -74.1648
Visiting point 41.8804, -74.4103
Visiting point 40.9146, -74.6189
Visiting point 40.0342, -74.8789
Visiting point 41.6855, -74.4421
Visiting point 41.9018, -74.9143
Visiting point 41.3628, -74.9016
Visiting point 40.1263, -74.7964
Visiting point 40.8123, -74.3999
Finished trip with 11 points
Passed 8 features
Travelled 817791 meters
It took 9 seconds
-------------- RouteChat --------------
Sending message First message at 0, 0
Sending message Second message at 0, 1
Sending message Third message at 1, 0
Sending message Fourth message at 0, 0
Got message First message at 0, 0
```
