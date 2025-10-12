#!/usr/bin/env bash
# ADS-B One-Shot Setup (readsb + tar1090) for Debian 13 (Trixie)
# Author: HB9IU  |  License: Public Domain

set -euo pipefail

# ---------- Colors ----------
if [[ -t 1 ]]; then
  RED=$'\033[31m'; GRN=$'\033[32m'; YLW=$'\033[33m'; BLU=$'\033[34m'
  CYN=$'\033[36m'; MAG=$'\033[35m'; BLD=$'\033[1m'; RST=$'\033[0m'
else
  RED= GRN= YLW= BLU= CYN= MAG= BLD= RST=
fi

# ---------- Helpers ----------
need_root() { [[ "$(id -u)" = 0 ]] || { echo "Please run with sudo:  sudo $0"; exit 1; }; }
fetch() { command -v wget >/dev/null && wget -nv -O - "$1" || curl -fsSL "$1"; }
is_number() { [[ "$1" =~ ^-?[0-9]+([.][0-9]+)?$ ]]; }
in_range() { awk -v v="$1" -v lo="$2" -v hi="$3" 'BEGIN{exit !(v>=lo && v<=hi)}'; }
clear_screen() { command -v clear >/dev/null && clear || printf '\033[2J\033[H'; }

intro() {
  clear_screen
  echo "${BLD}${CYN}=== ADS-B Setup (readsb + tar1090) — by HB9IU ===${RST}"
  echo "${YLW}This will:${RST}"
  echo "  • Update APT and install base packages"
  echo "  • Install ${BLD}readsb${RST} (decoder)"
  echo "  • Ask you for ${BLD}Latitude / Longitude / Gain${RST} and apply them"
  echo "  • Install ${BLD}tar1090${RST} (web UI)"
  echo

  # Animation only on TTY, and on a single line using \r (no cursor hopping)
  [[ -t 1 ]] || { echo; return; }

  cols=$(tput cols 2>/dev/null || echo 80)
  (( cols < 30 )) && cols=80
  line=$(printf '%*s' "$cols" '' | tr ' ' '─')
  echo "${BLU}${line}${RST}"
  msg="${MAG}Fasten seatbelts — preparing for takeoff…${RST}"
  echo "$msg"

  plane="✈"
  for ((i=1;i<=cols-2;i++)); do
    printf "\r${GRN}%*s${RST}" "$i" "$plane"
    sleep 0.01
  done
  printf "\r${GRN}%s${RST}\n" "$plane"
  echo "${GRN}Ready!${RST}"
  echo
}

# ---------- Start ----------
need_root
intro

# -------- Prompts --------
DEFAULT_LAT="46.46682264133937"
DEFAULT_LON="6.861620535525932"
DEFAULT_GAIN="49.6"   # or 'auto'

read -rp "Enter Latitude  [default ${DEFAULT_LAT}]: " LAT
LAT="${LAT:-$DEFAULT_LAT}"
read -rp "Enter Longitude [default ${DEFAULT_LON}]: " LON
LON="${LON:-$DEFAULT_LON}"
read -rp "Enter Gain (auto or number, default ${DEFAULT_GAIN}): " GAIN
GAIN="${GAIN:-$DEFAULT_GAIN}"

# -------- Validation --------
if ! is_number "$LAT" || ! in_range "$LAT" -90 90; then
  echo "${RED}Error:${RST} Latitude must be a number between -90 and 90."; exit 1; fi
if ! is_number "$LON" || ! in_range "$LON" -180 180; then
  echo "${RED}Error:${RST} Longitude must be a number between -180 and 180."; exit 1; fi
if [[ "$GAIN" != "auto" ]] && ! is_number "$GAIN"; then
  echo "${RED}Error:${RST} Gain must be 'auto' or a numeric value (e.g., 49.6)."; exit 1; fi

echo
echo "${BLD}Summary:${RST}"
printf "  Latitude : %s\n  Longitude: %s\n  Gain     : %s\n" "$LAT" "$LON" "$GAIN"
read -rp "Proceed? [Y/n]: " OK; OK="${OK:-Y}"
[[ "${OK^^}" = "Y" ]] || { echo "Aborted."; exit 1; }
echo

# -------- Install --------
echo "${CYN}==>${RST} Updating APT and installing prerequisites…"
apt-get update
apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  git build-essential pkg-config \
  librtlsdr-dev libusb-1.0-0-dev rtl-sdr \
  ca-certificates curl wget lighttpd

echo "${CYN}==>${RST} Installing readsb (decoder)…"
bash -c "$(fetch https://github.com/wiedehopf/adsb-scripts/raw/master/readsb-install.sh)"

echo "${CYN}==>${RST} Applying location and gain…"
readsb-set-location "$LAT" "$LON"
[[ "$GAIN" == "auto" ]] && readsb-gain auto || readsb-gain "$GAIN"

echo "${CYN}==>${RST} Installing tar1090 (web UI)…"
bash -c "$(fetch https://github.com/wiedehopf/tar1090/raw/master/install.sh)"

echo "${CYN}==>${RST} Restarting decoder…"
systemctl restart readsb

IP="$(hostname -I | awk '{print $1}')"
echo
echo "✅ ${BLD}Done.${RST} Open the map at:  ${GRN}http://${IP}/tar1090/${RST}"
echo "   or: ${GRN}http://$(hostname).local/tar1090/${RST}"
echo
echo "Useful commands:"
echo "  ${BLD}sudo readsb-set-location <lat> <lon>${RST}"
echo "  ${BLD}sudo readsb-gain auto${RST}   # or: ${BLD}sudo readsb-gain 49.6${RST}"
echo "  ${BLD}journalctl -u readsb -e${RST}"
