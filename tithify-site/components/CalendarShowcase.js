"use client";

import { useRef } from "react";
import Image from "next/image";
import { motion, useInView, useReducedMotion } from "framer-motion";
import { Eye, Move, Zap, Sparkles } from "lucide-react";
import { basePath } from "@/lib/basePath";

const easeOut = [0.16, 1, 0.3, 1];

export default function CalendarShowcase() {
  const sectionRef = useRef(null);
  const isVisible = useInView(sectionRef, { amount: 0.18, once: true });
  const prefersReducedMotion = useReducedMotion();

  return (
    <section
      id="preview"
      ref={sectionRef}
      className="relative overflow-hidden bg-white pt-16 pb-20 md:pt-20 md:pb-28"
    >
      <div id="calendar" className="absolute top-0 left-0" />
      <div className="absolute inset-y-0 right-0 w-1/2 bg-gradient-to-l from-sapphire-50/60 to-transparent pointer-events-none" />

      <div className="relative mx-auto grid max-w-7xl items-center gap-12 px-6 md:px-10 lg:min-h-[760px] lg:grid-cols-[minmax(0,0.92fr)_minmax(0,1.08fr)] lg:gap-10">
        <motion.div
          initial={prefersReducedMotion ? false : { opacity: 0, x: -24 }}
          whileInView={{ opacity: 1, x: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.6, ease: easeOut }}
          className="relative z-[1] max-w-xl lg:max-w-[32rem]"
        >
          <span className="inline-flex items-center gap-2 text-sm font-semibold tracking-[0.18em] uppercase text-sapphire-600">
            <Sparkles className="w-3.5 h-3.5 text-crimson" />
            Nepali Calendar Widget &amp; Desktop Patro
          </span>
          <h2 className="mt-4 font-display text-4xl font-extrabold tracking-tight text-slate-900 md:text-5xl">
            Interactive Nepali Calendar Widget,
            <br />
            right above your Windows taskbar.
          </h2>
          <p className="mt-5 text-lg leading-relaxed text-slate-600">
            Click this Windows widget to instantly open the interactive Bikram Sambat (वि.सं.) calendar. View Nepali government public holidays, festivals, and BS dates across 1975 to 2100 BS without opening a browser.
          </p>

          <div className="mt-8 space-y-5">
            <Feature icon={Eye} title="Instant At-A-Glance Date">
              Always stay oriented with today's Bikram Sambat date at a glance—no browser opening or app switching needed.
            </Feature>
            <Feature icon={Move} title="Freely Position & Lock">
              Right-click to unlock and place the widget anywhere above your taskbar or screen edge, then lock it for click-through convenience.
            </Feature>
            <Feature icon={Zap} title="Pure Native & Zero Distraction">
              Crafted with true per-pixel transparency and smooth acrylic glass effects that blend naturally into your Windows setup.
            </Feature>
          </div>

          <div className="mt-8 flex flex-wrap items-center gap-2 pt-2 text-xs font-medium text-slate-500">
            <span className="rounded-full bg-slate-100 px-3 py-1 text-slate-700 border border-slate-200/60">Bikram Sambat (वि.सं.)</span>
            <span className="rounded-full bg-slate-100 px-3 py-1 text-slate-700 border border-slate-200/60">Zero CPU Overhead</span>
            <span className="rounded-full bg-slate-100 px-3 py-1 text-slate-700 border border-slate-200/60">Always On Top</span>
            <span className="rounded-full bg-slate-100 px-3 py-1 text-slate-700 border border-slate-200/60">100% Offline</span>
          </div>
        </motion.div>

        <motion.div
          initial={prefersReducedMotion ? false : { opacity: 0, x: 36, scale: 0.97 }}
          whileInView={{ opacity: 1, x: 0, scale: 1 }}
          viewport={{ once: true, margin: "-100px" }}
          transition={{ duration: 0.85, delay: 0.08, ease: easeOut }}
          whileHover={prefersReducedMotion ? undefined : { y: -6, scale: 1.01 }}
          className="relative min-w-0 w-full flex items-center justify-center"
        >
          <div className="relative aspect-[3538/2208] w-full drop-shadow-2xl">
            <div
              className="absolute z-0 overflow-hidden bg-slate-950"
              style={{
                left: "11.8%",
                top: "4.5%",
                width: "75.8%",
                height: "74.0%",
              }}
            >
              {isVisible && (
                <video
                  className="h-full w-full object-cover object-bottom"
                  src={`${basePath}/Live%20preview%20of%20widget.mp4`}
                  aria-label="Live preview of Tithify"
                  autoPlay={!prefersReducedMotion}
                  controls={prefersReducedMotion}
                  loop
                  muted
                  playsInline
                  preload="metadata"
                >
                  Your browser does not support the product preview video.
                </video>
              )}
            </div>

            <Image
              src={`${basePath}/half-cutout-laptop.webp`}
              alt="Tithify full laptop preview"
              width={1600}
              height={998}
              sizes="(min-width: 1024px) 50vw, 100vw"
              className="pointer-events-none absolute inset-0 z-[1] h-full w-full object-contain select-none"
              loading="lazy"
            />
          </div>
        </motion.div>
      </div>
    </section>
  );
}

function Feature({ icon: Icon, title, children }) {
  return (
    <div className="flex items-start gap-3.5 text-sm leading-relaxed">
      <span className="flex h-9 w-9 shrink-0 items-center justify-center rounded-xl bg-sapphire-50 text-sapphire-600 mt-0.5">
        <Icon className="h-4 w-4" strokeWidth={2} />
      </span>
      <div>
        <h3 className="font-display font-bold text-slate-900 text-sm md:text-base">{title}</h3>
        <p className="mt-0.5 text-slate-600 text-sm leading-relaxed">{children}</p>
      </div>
    </div>
  );
}
