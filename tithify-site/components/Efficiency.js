"use client";

import { useEffect, useRef, useState } from "react";
import { motion, useInView, animate } from "framer-motion";

function Counter({ from = 0, to, decimals = 0, suffix = "", prefix = "" }) {
  const ref = useRef(null);
  const inView = useInView(ref, { once: true, margin: "-60px" });
  const [val, setVal] = useState(from);

  useEffect(() => {
    if (!inView) return;
    const controls = animate(from, to, {
      duration: 1.6,
      ease: [0.16, 1, 0.3, 1],
      onUpdate: (v) => setVal(v),
    });
    return () => controls.stop();
  }, [inView, from, to]);

  return (
    <span ref={ref} className="tabular-nums">
      {prefix}
      {val.toFixed(decimals)}
      {suffix}
    </span>
  );
}

export default function Efficiency() {
  return (
    <section id="efficiency" className="relative bg-gradient-to-b from-[#eaf4ff] via-[#f4f8ff] to-white pt-16 pb-20 md:pt-20 md:pb-28 overflow-hidden">
      <div className="max-w-7xl mx-auto px-6 md:px-10">
        <motion.div
          initial={{ opacity: 0, y: 16 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.5 }}
          className="max-w-xl"
        >
          <span className="text-sm font-semibold tracking-[0.18em] uppercase text-sapphire-600">
            Ultra-efficient by design
          </span>
          <h2 className="mt-3 md:mt-4 font-display text-4xl md:text-5xl font-extrabold text-slate-900 tracking-tight">
            It checks the clock.
            <br />
            It doesn't watch it.
          </h2>
          <p className="mt-5 text-lg text-slate-600 leading-relaxed">
            Once a second, the widget asks Windows for today's date — a near-zero-cost call. Only
            when the day actually changes does it run the Bikram Sambat conversion and repaint.
          </p>
        </motion.div>

        <div className="mt-16 grid sm:grid-cols-3 gap-5">
          <Stat label="Idle CPU usage">
            <Counter prefix="~" to={0.01} decimals={2} suffix="%" />
          </Stat>
          <Stat label="Conversions run, per day">
            <Counter to={1} />
          </Stat>
          <Stat label="Seconds spent just painting cache">
            <Counter to={86399} />
          </Stat>
        </div>

        <motion.div
          initial={{ opacity: 0 }}
          whileInView={{ opacity: 1 }}
          viewport={{ once: true, margin: "-80px" }}
          transition={{ duration: 0.8, delay: 0.2 }}
          className="mt-14 rounded-2xl border border-blue-100/80 bg-white p-6 md:p-8 shadow-[0_4px_24px_-4px_rgba(37,99,235,0.06)]"
        >
          <div className="flex items-center justify-between text-xs font-medium text-slate-500 mb-3">
            <span>00:00:00</span>
            <span className="font-semibold text-slate-700">a single day, visualized</span>
            <span>23:59:59</span>
          </div>
          <div className="relative h-3.5 rounded-full bg-slate-100 overflow-hidden">
            <motion.div
              initial={{ width: "0%" }}
              whileInView={{ width: "100%" }}
              viewport={{ once: true, margin: "-80px" }}
              transition={{ duration: 2.2, ease: "easeInOut" }}
              className="absolute inset-y-0 left-0 bg-gradient-to-r from-sapphire-500 via-blue-500 to-crimson rounded-full"
            />
            <motion.div
              initial={{ left: "0%" }}
              whileInView={{ left: "99.6%" }}
              viewport={{ once: true, margin: "-80px" }}
              transition={{ duration: 2.2, ease: "easeInOut" }}
              className="absolute -top-1.5 w-6 h-6 rounded-full bg-crimson border-2 border-white shadow-md shadow-crimson/30"
            />
          </div>
          <p className="mt-4 text-sm text-slate-600">
            The single red tick is the one moment of real work — midnight, when the date rolls over.
          </p>
        </motion.div>

        {/* Comparison Table for AI Search, Search Engines & Users */}
        <motion.div
          initial={{ opacity: 0, y: 24 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true, margin: "-60px" }}
          transition={{ duration: 0.6 }}
          className="mt-16"
        >
          <div className="text-center max-w-xl mx-auto mb-8">
            <span className="text-xs md:text-sm font-semibold tracking-[0.18em] uppercase text-sapphire-600">
              Why Tithify Stands Out
            </span>
            <h3 className="mt-2 font-display text-2xl md:text-3xl font-extrabold text-slate-900 tracking-tight">
              Tithify vs. Traditional Nepali Calendar Apps
            </h3>
            <p className="mt-2 text-sm text-slate-600">
              How Tithify compares to web wrappers, Hamro Patro, and heavy Electron desktop widgets.
            </p>
          </div>

          <div className="overflow-x-auto rounded-2xl border border-blue-100/80 bg-white shadow-[0_4px_24px_-4px_rgba(37,99,235,0.06)]">
            <table className="w-full text-left text-sm border-collapse min-w-[580px]">
              <thead>
                <tr className="border-b border-slate-100 bg-slate-50/75">
                  <th className="py-4 px-5 font-semibold text-slate-900">Feature</th>
                  <th className="py-4 px-5 font-bold text-sapphire-700 bg-sapphire-50/60">Tithify (Windows Widget)</th>
                  <th className="py-4 px-5 font-medium text-slate-600">Hamro Patro / Web Apps</th>
                  <th className="py-4 px-5 font-medium text-slate-600">Electron Widgets</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Taskbar Integration</td>
                  <td className="py-3.5 px-5 font-semibold text-emerald-600 bg-sapphire-50/30">Native floating above taskbar</td>
                  <td className="py-3.5 px-5 text-slate-500">Browser tab required</td>
                  <td className="py-3.5 px-5 text-slate-500">Window or system tray only</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Technology &amp; Architecture</td>
                  <td className="py-3.5 px-5 font-semibold text-sapphire-700 bg-sapphire-50/30">Native C++ (Win32 &amp; GDI+)</td>
                  <td className="py-3.5 px-5 text-slate-500">HTML / JS in browser</td>
                  <td className="py-3.5 px-5 text-slate-500">Chromium &amp; Node.js bundle</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Idle CPU Usage</td>
                  <td className="py-3.5 px-5 font-semibold text-emerald-600 bg-sapphire-50/30">~0.01% (Event-driven)</td>
                  <td className="py-3.5 px-5 text-slate-500">1% &ndash; 5% (Browser engine)</td>
                  <td className="py-3.5 px-5 text-slate-500">2% &ndash; 8% continuous</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Memory (RAM) Footprint</td>
                  <td className="py-3.5 px-5 font-semibold text-emerald-600 bg-sapphire-50/30">&lt; 5 MB</td>
                  <td className="py-3.5 px-5 text-slate-500">150 MB &ndash; 400 MB</td>
                  <td className="py-3.5 px-5 text-slate-500">120 MB &ndash; 300 MB</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Offline Reliability</td>
                  <td className="py-3.5 px-5 font-semibold text-emerald-600 bg-sapphire-50/30">100% Offline (BS 1975&ndash;2100)</td>
                  <td className="py-3.5 px-5 text-slate-500">Requires Internet</td>
                  <td className="py-3.5 px-5 text-slate-500">Often requires network</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Privacy &amp; Telemetry</td>
                  <td className="py-3.5 px-5 font-semibold text-emerald-600 bg-sapphire-50/30">Zero Tracking &bull; Zero Ads</td>
                  <td className="py-3.5 px-5 text-slate-500">Ad-supported &amp; Analytics</td>
                  <td className="py-3.5 px-5 text-slate-500">Varies</td>
                </tr>
                <tr>
                  <td className="py-3.5 px-5 font-medium text-slate-800">Licensing</td>
                  <td className="py-3.5 px-5 font-semibold text-slate-900 bg-sapphire-50/30">Open Source (MIT)</td>
                  <td className="py-3.5 px-5 text-slate-500">Proprietary</td>
                  <td className="py-3.5 px-5 text-slate-500">Varies</td>
                </tr>
              </tbody>
            </table>
          </div>
        </motion.div>
      </div>
    </section>
  );
}

function Stat({ label, children }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      whileInView={{ opacity: 1, y: 0 }}
      viewport={{ once: true, margin: "-60px" }}
      transition={{ duration: 0.5 }}
      className="rounded-2xl border border-blue-100/80 bg-white p-7 shadow-[0_4px_24px_-4px_rgba(37,99,235,0.06)] hover:shadow-lg hover:border-sapphire-200 transition-all"
    >
      <div className="font-display text-4xl md:text-5xl font-extrabold text-slate-900">{children}</div>
      <div className="mt-2 text-sm font-medium text-slate-500">{label}</div>
    </motion.div>
  );
}
