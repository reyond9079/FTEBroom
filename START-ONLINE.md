# ReyondBroom online: quick start for two people

This is an experimental release. Extract it into a **new writable folder**, separate
from the regular editor. Each participant needs their own installation. For two
clients on one computer, unpack the archive twice before joining either client.

## 1. Prepare both editors

- Install Python 3.10 or newer. On Windows, enable the Python PATH option, or note
  the full path to python.exe. Linux needs Python 3 and the SDL2 runtime.
- Start ReyondBroom.exe on Windows, or Launch-ReyondBroom.sh on Linux.
- Setup now goes through language, interface size, theme, **server list**, and Discord.
  There is no game/mod selection. LibreQuake and BSP2 are fixed for this online build;
  missing LibreQuake resources download automatically when you join.
- Save any current map before joining. Joining opens your assigned online MAP.

## 2. Start a master and one relay

There is no hosted public master yet. GitHub stores the code and downloads, but
GitHub Pages cannot execute the directory service. Run it on one LAN/VPN computer.
The master only lists relays; editing data travels directly to the selected relay.

Open a terminal in the unpacked editor folder:

```sh
python3 ./ftebroom/online/master.py --host 0.0.0.0 --port 28776 --allow-private
```

On Windows use `python` instead of `python3`. Leave this terminal open. The
`--allow-private` flag explicitly enables local/LAN/VPN relay addresses.

Before starting a relay for the first time, open **Hosting / directory setup** in
the editor's server list and choose **Prepare LibreQuake for hosting**. Wait until
it says LibreQuake is installed. This does not need a running relay.

Only one participant starts the server. Open a terminal in that participant's
unpacked editor folder. Run the command below and leave the terminal open.

Windows (PowerShell):

```powershell
python .\ftebroom\online\server.py --host 0.0.0.0 --port 28777 --data .\online-server-data --librequake .\downloads\librequake-v0.09\mod_lite\lq1 --master http://192.168.1.50:28776 --name "Our workshop"
```

If `python` is unavailable, use `py -3` or the full path to python.exe instead.
In PowerShell, invoke a quoted executable path with `&`, for example
`& "C:\Path\To\python.exe" .\ftebroom\online\server.py ...`.

Linux:

```sh
python3 ./ftebroom/online/server.py --host 0.0.0.0 --port 28777 --data ./online-server-data --librequake ./downloads/librequake-v0.09/mod_lite/lq1 --master http://192.168.1.50:28776 --name "Our workshop"
```

The LibreQuake folder above appears after setup finishes downloading the mod. If
you installed it elsewhere, replace only the `--librequake` path.

Replace `192.168.1.50` with the master's real LAN/VPN IP, even when the master and
relay run on the same computer. A relay registering through localhost is listed
as localhost and is only useful for a same-computer test. Every host using the same `--master` automatically
appears in the same list. Omit `--master` for an unlisted direct-connect relay.
Registration refreshes every 30 seconds; stopped/unreachable relays expire after
90 seconds. The master verifies the relay at its actual connection address; it
does not provide NAT traversal or tunnel traffic.

If your relay already has an HTTPS tunnel (for example a Cloudflare quick tunnel),
append `--public-url https://YOUR-TUNNEL.trycloudflare.com` to its command. The
master verifies `/info` through that URL and lists the HTTPS address instead of
the local IP. Keep the tunnel running; if its address changes, restart the relay
with the new URL. You can also expose the master through a separate tunnel and
use that origin in `--master` and in both editors. A relay URL is not a master URL:
the relay serves `/info`, the master serves `/servers` and `/register`.

These commands accept LAN connections. Use a trusted local network or private VPN;
the prototype is not a public Internet service. If the host firewall asks, allow
Python on the private network, or allow incoming TCP ports 28776 (master) and
28777 (relay). No router port
forwarding is needed for two computers on the same LAN.

For a same-computer-only test, replace `--host 0.0.0.0` with `--host 127.0.0.1`.
Stop the server with Ctrl+C. Keep online-server-data to retain assigned plots/maps.

## 3. Connect both participants

In each editor, open **Workspace > Online session (experimental)**:

The same controls appear during first-time setup. Set **Master server** to
`http://127.0.0.1:28776` on the host and `http://HOST-LAN-IP:28776` on the other
computer, then click **Refresh servers** and select **Our workshop**. The master
address, name, Python path and selected/direct address are saved in that editor.
If the master is unavailable, use the **Direct server / selected** field instead:

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
- Refresh the master list, select the relay, and reconnect after restarting the editor.
- Stop a relay and refresh after 90 seconds: its listing should disappear.
- Publish a simple open platform and see it in the other editor.
- Add a brush and compile again while the other plot is visible: only your plot changes.
- Check out-of-plot rollback, Undo/Redo, MAP/BSP switching and reconnection.
- Disconnect before returning to ordinary mapping. Automatic editor updates are
  disabled in this experimental branch so it cannot silently become a regular build.

If connection fails, check the server terminal, matching resource packs, host IP,
port/firewall and Python executable. The client also writes status messages to
ftebroom/data/online/status.txt. More details and limits: [docs/online.md](docs/online.md).
