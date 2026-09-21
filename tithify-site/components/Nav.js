"use client";

import { useEffect, useState } from "react";
import { motion, useScroll, useTransform } from "framer-motion";
import Image from "next/image";
import { basePath } from "@/lib/basePath";

export default function Nav() {
  const { scrollY } = useScroll();

  // The Hero section is h-[190vh]. Capture the exact pixel position of the Features
  // section so the width expansion triggers precisely when the user arrives there.
  const [featuresStart, setFeaturesStart] = useState(3000);
  useEffect(() => {
    const update = () => setFeaturesStart(window.innerHeight * 1.9);
    update();
    window.addEventListener("resize", update);
    return () => window.removeEventListener("resize", update);
  }, []);

  // Helper — linear interpolation clamped to [0,1]
  const lerp = (a, b, t) => a + (b - a) * Math.min(1, Math.max(0, t));

  // Transition starts 350px BEFORE the Features section and spreads over 500px,
  // giving a long, gentle fade that begins while still in the lower Hero and
  // completes gracefully just after Features scrolls into view.
  const makePct = () =>
    Math.min(1, Math.max(0, (scrollY.get() - (featuresStart - 350)) / 500));

  const bg = useTransform(scrollY, () => {
    const t = makePct();
    // dark glass → frosted white
    const r = Math.round(lerp(11,  255, t));
    const g = Math.round(lerp(17,  255, t));
    const b = Math.round(lerp(32,  255, t));
    const a =            lerp(0.65, 0.92, t);
    return `rgba(${r}, ${g}, ${b}, ${a.toFixed(2)})`;
  });

  const border = useTransform(scrollY, () => {
    const t = makePct();
    const r = Math.round(lerp(255, 226, t));
    const g = Math.round(lerp(255, 232, t));
    const b = Math.round(lerp(255, 240, t));
    const a =            lerp(0.12, 0.95, t);
    return `rgba(${r}, ${g}, ${b}, ${a.toFixed(2)})`;
  });

  const shadow = useTransform(scrollY, () => {
    const t = makePct();
    if (t < 0.5) return "0 4px 20px -5px rgba(0, 0, 0, 0.3)";
    return "0 10px 30px -10px rgba(15, 23, 42, 0.08)";
  });

  // Brand name and nav links: white on dark hero → dark on light sections
  const brandText = useTransform(scrollY, () => {
    const t = makePct();
    const r = Math.round(lerp(255,  15, t));
    const g = Math.round(lerp(255,  23, t));
    const b = Math.round(lerp(255,  42, t));
    return `rgb(${r}, ${g}, ${b})`;
  });

  const linkText = useTransform(scrollY, () => {
    const t = makePct();
    const r = Math.round(lerp(203, 51, t));
    const g = Math.round(lerp(213, 65, t));
    const b = Math.round(lerp(225, 85, t));
    return `rgb(${r}, ${g}, ${b})`;
  });

  // Width starts cropped (62rem) so the celestial moon is visible in the Hero.
  // Only expands to full (80rem) once the user scrolls past the 190vh Hero
  // and the Features section reaches the top. The 80px window gives a smooth
  // Width uses the exact same timing as the color/text transitions above.
  const maxWidthPx = useTransform(
    scrollY,
    () => {
      const pct = makePct();
      const rem = 62 + (80 - 62) * pct;
      return `${rem}rem`;
    }
  );

  return (
    <motion.header
      initial={{ y: -60, opacity: 0 }}
      animate={{ y: 0, opacity: 1 }}
      transition={{ duration: 0.7, ease: [0.16, 1, 0.3, 1] }}
      className="fixed top-0 left-0 right-0 z-50 px-6 md:px-10 py-3.5"
    >
      <div className="max-w-7xl mx-auto flex">
        <motion.div
          style={{
            backgroundColor: bg,
            borderColor: border,
            boxShadow: shadow,
            maxWidth: maxWidthPx,
          }}
          className="w-full flex items-center justify-between rounded-2xl border px-4 py-2 backdrop-blur-xl"
        >
          <a href="#top" className="flex items-center gap-2 focus-ring">
            <Image
              src={`${basePath}/logo-mark.webp`}
              alt="Tithify logo"
              width={32}
              height={32}
              className="w-8 h-8"
              priority
            />
            <motion.span style={{ color: brandText }} className="font-display font-bold text-sm tracking-wide">
              Tith<span className="text-crimson font-extrabold">ify</span>
            </motion.span>
          </a>

          <nav className="hidden md:flex items-center gap-7 text-sm font-medium">
            <motion.a
              style={{ color: linkText }}
              href="#features"
              className="hover:!text-crimson transition-colors focus-ring"
            >
              Features
            </motion.a>
            <motion.a
              style={{ color: linkText }}
              href="#preview"
              className="hover:!text-crimson transition-colors focus-ring"
            >
              Preview
            </motion.a>
            <motion.a
              style={{ color: linkText }}
              href="#efficiency"
              className="hover:!text-crimson transition-colors focus-ring"
            >
              Performance
            </motion.a>
            <motion.a
              style={{ color: linkText }}
              href="#faq"
              className="hover:!text-crimson transition-colors focus-ring"
            >
              FAQ
            </motion.a>
            <motion.a
              style={{ color: linkText }}
              href="#donate"
              className="hover:!text-crimson transition-colors focus-ring"
            >
              Donate
            </motion.a>
          </nav>

          <a
            href="#install"
            className="focus-ring rounded-lg bg-crimson hover:bg-crimson-dark transition-all px-4 py-2 text-sm font-semibold text-white shadow-sm shadow-crimson/30 hover:scale-[1.02] active:scale-[0.98]"
          >
            Download
          </a>
        </motion.div>
      </div>
    </motion.header>
  );
}
