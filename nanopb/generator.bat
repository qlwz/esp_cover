cd /d %~dp0

D:\Workspaces_Smarthome\esp\nanopb\generator-bin\protoc.exe --nanopb_out=. CoverConfig.proto
copy CoverConfig.pb.h ..\include /y
copy CoverConfig.pb.c ..\src /y
del CoverConfig.pb.h
del CoverConfig.pb.c
