<h1 align="center">🌑 DARKGEN_PROFILER</h1>

<p align="center">
  <img src="https://img.shields.io/badge/Version-8.0-red.svg">
  <img src="https://img.shields.io/badge/Language-C-blue.svg">
  <img src="https://img.shields.io/badge/Platform-Termux%20%7C%20Kali-lightgrey.svg">
  <img src="https://img.shields.io/badge/License-MIT-green.svg">
</p>

<hr>

<p align="center">
  <b>Darkgen Profiler</b> is a high-performance intelligence suite designed to create personalized password lists. By utilizing target information like names, dates, and pet names, it crafts highly probable wordlists for security auditing and research.
</p>

---

## 🛠️ Key Features
- **🎯 Target Profiling:** Advanced mutations based on personal intelligence.
- **⚡ High Performance:** Multi-threaded engine written in pure C.
- **🧹 FNV-1a Dedup:** Integrated hash-table to remove duplicate entries instantly.
- **📦 GZip Compression:** Level 9 compression to save storage on massive lists.
- **📊 Real-time Stats:** Track password composition (Alpha, Digit, Special, Length).

---

## 🚀 Installation & Usage

### 📱 Termux (Android)
```bash
# Update packages
pkg update && pkg upgrade

# Install build tools and zlib
pkg install gcc make zlib binutils -y

# Compile the source
gcc darkgen.c -o darkgen -lpthread -lm -lz -O3

# Launch Darkgen
./darkgen

# Update system
sudo apt update

# Install build-essential and zlib headers
sudo apt install build-essential zlib1g-dev -y

# Compile the source
gcc darkgen.c -o darkgen -lpthread -lm -lz -O3

# Launch Darkgen
./darkgen

Mode Action
1. Custom Profiler Full intelligence gathering and mutation.
2. Intel Merge Mixes target info with 500+ common passwords.
3. God-List Generates high-entropy "Elite" password lists.
4. Brute-Force Recursive charset generation (custom length).
5. Mutator Import an existing list and apply Darkgen rules.

👤 Author
Shravan Acharya
GitHub: techpythoncode-bit
Email: techpythoncode@gmail.com
<p align="center">
<i>Developed for ethical security testing and research purposes only.</i>
</p>