#!/usr/bin/env bash
set -euo pipefail

# === install-samba-rootfs.sh ===
# Installs Samba, creates user 'daniel' with password 'a',
# and shares the entire filesystem (/) with R/W as root via SMB.
# USE AT YOUR OWN RISK.

REQUIRED_USER="daniel"
USER_PASSWORD="a"
SMB_CONF="/etc/samba/smb.conf"
BACKUP="/etc/samba/smb.conf.bak.$(date +%Y%m%d-%H%M%S)"

if [[ $EUID -ne 0 ]]; then
  echo "Please run as root: sudo $0"
  exit 1
fi

echo "[1/6] Installing Samba..."
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y samba

echo "[2/6] Ensuring local user '${REQUIRED_USER}' exists..."
if id -u "${REQUIRED_USER}" >/dev/null 2>&1; then
  echo "  - User ${REQUIRED_USER} already exists."
else
  adduser --disabled-password --gecos "" "${REQUIRED_USER}"
fi

echo "  - Setting local password for ${REQUIRED_USER}..."
echo "${REQUIRED_USER}:${USER_PASSWORD}" | chpasswd

echo "[3/6] Creating Samba user '${REQUIRED_USER}'..."
# Add to Samba (idempotent: delete then add to be sure)
(pdbedit -L | grep -q "^${REQUIRED_USER}:") && smbpasswd -x "${REQUIRED_USER}" || true
printf "%s\n%s\n" "${USER_PASSWORD}" "${USER_PASSWORD}" | smbpasswd -a "${REQUIRED_USER}"
smbpasswd -e "${REQUIRED_USER}"

echo "[4/6] Backing up existing smb.conf to ${BACKUP} (if present)..."
if [[ -f "${SMB_CONF}" ]]; then
  cp -a "${SMB_CONF}" "${BACKUP}"
fi

echo "[5/6] Writing new smb.conf (full rootfs share, R/W as root)..."
cat > "${SMB_CONF}" <<'EOF'
[global]
   workgroup = WORKGROUP
   server role = standalone server
   security = user
   map to guest = Never
   # Optional hardening/clarity
   disable netbios = yes
   smb ports = 445
   server min protocol = SMB2
   server max protocol = SMB3
   # Avoid issues with wide links (not recommended, but helps when sharing /)
   unix extensions = no
   allow insecure wide links = yes
   follow symlinks = yes
   wide links = yes

   # Logging (optional)
   log file = /var/log/samba/log.%m
   max log size = 1000

# === DANGEROUS SHARE: full filesystem as root ===
[rootfs]
   path = /
   browseable = yes
   read only = no
   writable = yes
   guest ok = no
   valid users = daniel
   admin users = daniel
   force user = root
   force group = root
   create mask = 0777
   directory mask = 0777
   follow symlinks = yes
   wide links = yes
EOF

echo "[6/6] Testing config and restarting Samba..."
testparm -s || { echo "Samba config test failed."; exit 1; }

systemctl enable smbd >/dev/null 2>&1 || true
systemctl restart smbd

# Print connection hint
IP=$(hostname -I | awk '{print $1}')
echo
echo "Done."
echo "Connect from another machine using:"
echo "  \\\\${IP}\\rootfs    (Windows)"
echo "  smb://$IP/rootfs    (macOS/Linux)"
echo
echo "Username: ${REQUIRED_USER}"
echo "Password: ${USER_PASSWORD}"
echo
echo "REMEMBER: This grants root-level R/W to your entire Pi over SMB."
