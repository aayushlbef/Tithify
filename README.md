# Tithify — Nepali Date Windows Taskbar Widget

<p align="center">
  <a href="https://tithify.guptaaayush.com.np"><strong>🌐 Official Website: tithify.guptaaayush.com.np</strong></a>
</p>

<p align="center">
  <img src="tithify-site/public/logo.png" alt="Tithify Logo" width="110" />
  &nbsp;&nbsp;&nbsp;&nbsp;
  <img src="tithify-site/public/widiget_transparent_shot.png" alt="Tithify Taskbar Widget Preview" width="280" />
</p>

<p align="center">
  A lightweight, native C++ desktop widget for Windows that beautifully displays the current Nepali (Bikram Sambat) date, specially designed to be viewed directly above your taskbar.
</p>

<p align="center">
  <img src="tithify-site/public/desktop-widget-showcase.png" alt="Tithify Desktop Widget Showcase" width="750" />
</p>

---

## 🚀 Features

- **Theme Auto-Adapt:** Automatically detects your Windows Light/Dark mode and adjusts its colors and text rendering seamlessly. Reacts to live theme changes instantly without a restart.
- **Auto-Updates:** Built-in update checker uses the GitHub Releases API on a background thread to notify you when a new version is available.
- **Customizable Layout:** Toggle the Day of the Week (Sun, Mon, Tue) next to the Nepali flag directly from the context menu.
- **Zero-Distraction Design:** Frameless, borderless, and perfectly transparent with high-quality GDI+ per-pixel alpha blending.
- **Smart Auto-Hide:** Automatically detects when you're playing games or watching movies in fullscreen and hides itself so it doesn't get in your way.
- **DPI Aware:** Renders crystal-clear text and graphics on high-resolution (4K) monitors without blurriness.
- **Desktop Z-Index Control:** When locked to the desktop, toggle the **Z-Index** option in the context menu to choose whether Tithify stays in front of open applications (Always on Top) or rests behind them on the desktop wallpaper.
- **Professional Installer:** Comes with a standard Windows Installer (`Setup.exe`) for easy installation, Start Menu shortcuts, and clean uninstalls (no Admin privileges required).
- **System Tray Icon:** Accessible from the system tray with a right-click context menu for quick access to all settings.

---

## 📅 Interactive Calendar Popup

Click the widget to open a full **Bikram Sambat Calendar** popup — powered by a native GDI+ rendering engine.

<p align="center">
  <img src="tithify-site/public/calendar-window.png" alt="Bikram Sambat Interactive Calendar Popup" width="350" />
</p>

- **Month Navigation:** Use the `‹` / `›` buttons or scroll the **mouse wheel** to navigate between months of the current BS year.
- **Dual Date Display:** Every calendar cell shows both the **BS day** (large) and the corresponding **AD date** (small, bottom-right).
- **Today Highlight:** The current day is highlighted in crimson red for instant recognition.
- **Keyboard Navigation:** Use **Arrow Keys** to move between days, **Home** to jump to today, and **Escape** to close.
- **Bilingual Header:** Month name displayed in both English and Devanagari script, e.g., `Baisakh 2082 • बैशाख २०८२`.
- **Gregorian Range:** Shows the full Gregorian (AD) date range for the displayed BS month.
- **Public Holiday & Saturday Indicators:** Saturdays and public holidays are highlighted in red; festival/event days show a colored dot indicator.

---

## 🎉 Holiday & Festival Engine

The calendar automatically fetches and displays Nepali public holidays and cultural events for the current BS year.

- **Online Auto-Fetch:** On first launch, holiday data is downloaded from GitHub (with jsDelivr CDN as a fallback) in a background thread — the UI never freezes.
- **Smart Local Cache:** Downloaded holidays are saved in an XOR-encoded binary `.dat` file under `%LOCALAPPDATA%\Tithify\` so they load instantly on every subsequent launch.
- **Event Flyout Panel:** Clicking on any day with an event shows a speech-bubble flyout with:
  - Full Nepali (Devanagari) and English event title
  - Event category badge (e.g., **PUBLIC HOLIDAY**, **FESTIVAL / पर्व**)
  - A detailed event description
- **Offline Mode:** If no internet is available, the calendar shows an **⚠ OFFLINE** notice with a **↻ Retry** button to re-attempt fetching when back online.
- **BS Year Coverage:** Full calendar data from **BS 1975 to 2100**.

---

## 💖 Support & Donate

Tithify is 100% free and open-source. If you find it helpful for your daily workflow, consider supporting its maintenance and ongoing development!

<div align="center">

| <img src="tithify-site/public/donate/esewa-logo.png" alt="eSewa Logo" height="36" /> | <img src="tithify-site/public/donate/khalti-logo.png" alt="Khalti Logo" height="36" /> |
| :---: | :---: |
| <img src="tithify-site/public/donate/esewa-qr.jpg" alt="eSewa QR Code" width="220" /> | <img src="tithify-site/public/donate/khalti-qr.png" alt="Khalti QR Code" width="220" /> |
| **eSewa Mobile Wallet** | **Khalti Digital Wallet** |

</div>

---

## ⚡ Ultra-Efficient Resource Management

This widget is engineered from the ground up to consume minimal system resources (~0.01% CPU usage and <5MB RAM).

**How it achieves this:**
Instead of constantly running complex date conversion algorithms, the widget uses an ultra-efficient caching technique:
1. Every second, it asks the Windows OS for the current Gregorian (AD) Date — a nearly zero-cost operation.
2. It checks if the Day, Month, Year has changed since the last frame.
3. Only if the day has physically changed (which happens exactly once a day at 12:00 AM) will the widget perform the Bikram Sambat conversion logic and re-render the text.
4. For the other 86,399 seconds of the day, it skips all math entirely and just paints the cached text to the screen!

This ensures you have a perfectly accurate, real-time widget that doesn't drain your laptop's battery or steal CPU cycles from your games.

---

## 🛠️ How It Works

The widget uses the native Windows Win32 API and GDI+ to render directly to your screen:
- **Z-Order Enforcement:** It uses a 250ms background timer to assert its `HWND_TOPMOST` status, preventing Windows from hiding it behind other windows.
- **Taskbar Previews:** It aggressively intercepts the `WM_SHOWWINDOW` API call to veto `SW_PARENTCLOSING` requests, stopping the Windows Taskbar thumbnail system from accidentally hiding it when you hover over open apps.
- **Native BS Conversion:** The entire Bikram Sambat (1975–2100) calendar mapping is embedded directly into the C++ binary. This guarantees offline date display out-of-the-box.
- **WinHTTP API:** Update checks and holiday fetching are performed safely on detached background threads using native Windows networking to ensure the UI never stutters.
- **Live Theme Sync:** Listens for `WM_SETTINGCHANGE` (ImmersiveColorSet) to detect and apply Windows theme changes in real-time.

---

## 🏃 Installation

### ⚡ Option 1: Fast Command-Line Install (PowerShell)

Open PowerShell and run the one-line command below:

```powershell
irm https://raw.githubusercontent.com/aayushlbef/Tithify/main/install.ps1 | iex
```

> **Command Prompt (CMD):**
> ```cmd
> powershell -c "irm -useb https://raw.githubusercontent.com/aayushlbef/Tithify/main/install.ps1 | iex"
> ```

### 📦 Option 2: Windows Package Manager (Winget) — _Coming Soon_

```powershell
winget install Aayush.Tithify
```

---

### 💾 Option 3: Manual Download

> [!NOTE]
> **Browser & Windows SmartScreen Warnings**
> Because this is a free open-source project without a paid EV code-signing certificate, your web browser or Windows SmartScreen might initially show a warning for new releases. You can safely bypass it by clicking **Keep -> Keep anyway** in your browser, and **More Info -> Run Anyway** in Windows.

1. Download **`Tithify_v3.6.4.zip`** or `Tithify_Setup.exe` from the [Latest Release](https://github.com/aayushlbef/Tithify/releases/latest).
2. Run `Tithify_Setup.exe` to install.

---

## 🎯 Getting Started & Usage

1. **Positioning:** Right-click the widget to unlock its position and drag it to your preferred spot on your screen / taskbar.
2. **Locking:** Right-click again and select **"Lock Position"** to make it click-through and seamless!
3. **Open Calendar:** **Left-click** the widget at any time to open the **interactive Bikram Sambat Calendar**.
4. **Settings:** Right-click to configure **Run at Startup**, toggle **Show Day of Week**, toggle **Z-Index** (when locked to desktop), switch themes, or check for updates.

---

## 🤝 How to Contribute

Contributions are welcome and appreciated! Whether it's a bug fix, a new feature, or improved documentation, here's how to get started.

### 🧰 Prerequisites

- **Windows 10/11** (the widget relies on the Win32 API and GDI+)
- **Visual Studio 2022** (or later) with the **Desktop development with C++** workload installed
- **Git**

### 🛠️ Local Development

1. **Fork** the repository and clone your fork:
   ```bash
   git clone https://github.com/aayushlbef/Tithify.git
   cd Tithify
   ```
2. Open the project in **Visual Studio** (use the solution/`.vcxproj` in this repo).
3. Select the **Release** (or **Debug**) configuration and build.
4. Run the compiled executable to test your changes.

### 🌱 Making Changes

1. Create a new branch for your work:
   ```bash
   git checkout -b feature/my-improvement
   ```
2. Make your changes, keeping commits focused and descriptive.
3. Test thoroughly on both **Light** and **Dark** Windows themes.

### 📤 Submitting

1. Push your branch to your fork:
   ```bash
   git push origin feature/my-improvement
   ```
2. Open a **Pull Request** against the `main` branch of this repository.
3. Describe what you changed and why — include screenshots for UI changes where possible.

### 🐞 Reporting Issues

- Search [existing issues](https://github.com/aayushlbef/Tithify/issues) before opening a new one.
- Include your **Windows version**, **widget version**, and clear steps to reproduce.

### 📜 License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).