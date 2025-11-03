# Accessing a Flask Server Running on WSL from Another Device

If your Flask app is running inside **WSL (Ubuntu on Windows)** and you want to access it from another device on the same network, you’ll need to **forward the port** from WSL to Windows.

---

## 1. Set Up Port Forwarding

Run **PowerShell as Administrator**, then execute:

```powershell
netsh interface portproxy add v4tov4 listenport=5000 listenaddress=0.0.0.0 connectport=5000 connectaddress=<WSL_IP>
```

> 🔸 Replace `<WSL_IP>` with the IP address of your WSL instance.  
> You can find it by running `hostname -I` inside WSL.  
> (Port **5000** is Flask’s default.)

---

## 2. Allow Firewall Access

Still in PowerShell, add a firewall rule to allow incoming traffic:

```powershell
New-NetFirewallRule -DisplayName "WSL Flask Port 5000" -Direction Inbound -LocalPort 5000 -Protocol TCP -Action Allow
```

---

## 3. Access the Server

Once configured, you can access your Flask app from another device on the same LAN using your **Windows machine’s IP address**:

```
http://<Windows_IP>:5000
```

> Example: if your Windows IP is `192.168.1.10`, then open  
> `http://192.168.1.10:5000` from another computer or phone.

---

## 4. Remove (Disable) Port Forwarding

If you no longer need the setup, delete the port forwarding rule:

```powershell
netsh interface portproxy delete v4tov4 listenport=5000 listenaddress=0.0.0.0
```

To check all existing rules:

```powershell
netsh interface portproxy show all
```

---

## 5. Remove the Firewall Rule

If you previously added the firewall rule:

```powershell
Remove-NetFirewallRule -DisplayName "WSL Flask Port 5000"
```

---

## Summary

| Task | Command | Purpose |
|------|----------|----------|
| Add port forwarding | `netsh interface portproxy add ...` | Forward Windows port to WSL |
| Allow firewall access | `New-NetFirewallRule ...` | Enable network access |
| Remove port forwarding | `netsh interface portproxy delete ...` | Disable forwarding |
| Remove firewall rule | `Remove-NetFirewallRule ...` | Close external access |

---

> **Security Note:**  
> Opening a port with `listenaddress=0.0.0.0` exposes it to your entire local network. Only enable this when testing in a trusted environment and remember to remove the rules afterward.
