DiscordNameTool - OG style

Build in MSYS2 UCRT64:
g++ -std=c++17 main.cpp -o DiscordNameTool.exe -mwindows -municode -static -static-libgcc -static-libstdc++ -lwinpthread -luser32 -lgdi32 -lcomctl32

Run:
./DiscordNameTool.exe
