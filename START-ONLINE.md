# ReyondBroom online: quick start for two people

This is an experimental release. Extract it into a **new writable folder**, separate
from the regular editor. Each participant needs their own installation. For two
clients on one computer, unpack the archive twice before joining either client.

## 1. Prepare both editors

- Install Python 3.10 or newer. On Windows, enable the Python PATH option, or note
  the full path to python.exe. Linux needs Python 3 and the SDL2 runtime.
- Start ReyondBroom.exe on Windows, or Launch-ReyondBroom.sh on Linux.
- Select **Quake 1**, then **LibreQuake**, and finish loading resources. Both people
  must use the same LibreQuake version and unmodified PAK/WAD resource pack.
- Save any current map before joining. Joining opens your assigned online MAP.

## 2. Start one server

Only one participant starts the server. Open a terminal in that participant's
unpacked editor folder. Run the command below and leave the terminal open.

Windows (PowerShell):

```powershell
python .\ftebroom\online\server.py --host 0.0.0.0 --port 28777 --data .\online-server-data --librequake .\downloads\librequake-v0.09\mod_lite\lq1
```

If `python` is unavailable, use `py -3` or the full path to python.exe instead.
In PowerShell, invoke a quoted executable path with `&`, for example
`& "C:\Path\To\python.exe" .\ftebroom\online\server.py ...`.

Linux:

```sh
python3 ./ftebroom/online/server.py --host 0.0.0.0 --port 28777 --data ./online-server-data --librequake ./downloads/librequake-v0.09/mod_lite/lq1
```

The LibreQuake folder above appears after setup finishes downloading the mod. If
you installed it elsewhere, replace only the `--librequake` path.

These commands accept LAN connections. Use a trusted local network or private VPN;
the prototype is not a public Internet service. If the host firewall asks, allow
Python on the private network, or allow incoming TCP port 28777. No router port
forwarding is needed for two computers on the same LAN.

For a same-computer-only test, replace `--host 0.0.0.0` with `--host 127.0.0.1`.
Stop the server with Ctrl+C. Keep online-server-data to retain assigned plots/maps.

## 3. Connect both participants

In each editor, open **Workspace > Online session (experimental)**:

1. Server on the same computer: enter `http://127.0.0.1:28777`.
2. Server on another LAN computer: enter its IPv4 address, for example
   `http://192.168.1.50:28777`. Use the host's actual address, not the example.
   Windows shows it in `ipconfig`; Linux shows it in its network settings.
3. Enter different display names, for example Alice and Bob.
4. Python executable: `python` on Windows, `python3` on Linux, or its full executable
   path. This field takes an executable path only, not `py -3` or other arguments.
5. Click **Join session**. Each person receives a different cyan-outlined plot.

You can fly throughout the scene. Other participants appear as player models.
Geometry changes outside your own plot are rolled back. Do not copy one joined
installation's session.json into the other person's installation: it is their identity.

## 4. Build and share your plot

1. Edit your own plot, then **save** the map.
2. Click **Compile**. Only your MAP is compiled; other plots are render models and
   are never included in the compiler input.
3. In the online panel, click **Publish last saved MAP + compiled BSP**. Your partner
   receives the accepted BSP. Uncompiled edits are not synchronized live.
4. Use **View last published BSP** and **Back to editable MAP** to change views.

Online builds use BSP2, retain collision hulls, skip VIS and accept open/unsealed
maps (`-nofill`). LIGHT is optional. Your partner's MAP/BSP files are not rebuilt
or changed. Oversized, out-of-plot or foreign-resource BSPs are rejected.

## Check together

- Both people have different plots and can see each other's moving avatars.
- Publish a simple open platform and see it in the other editor.
- Add a brush and compile again while the other plot is visible: only your plot changes.
- Check out-of-plot rollback, Undo/Redo, MAP/BSP switching and reconnection.
- Disconnect before returning to ordinary mapping. Automatic editor updates are
  disabled in this experimental branch so it cannot silently become a regular build.

If connection fails, check the server terminal, matching resource packs, host IP,
port/firewall and Python executable. The client also writes status messages to
ftebroom/data/online/status.txt. More details and limits: [docs/online.md](docs/online.md).
