import Image from "next/image";
import { basePath } from "@/lib/basePath";

export default function Footer() {
  return (
    <footer className="bg-night-950 text-slate-400 py-14">
      <div className="max-w-7xl mx-auto px-6 md:px-10 flex flex-col md:flex-row items-center justify-between gap-6">
        <div className="flex items-center gap-1.5">
          <Image src={`${basePath}/logo-mark.webp`} alt="Tithify logo" width={32} height={32} className="w-8 h-8" loading="lazy" />
          <span className="font-display font-bold text-white text-sm">Tithify — Nepali Date Widget</span>
        </div>
        <p className="text-xs text-slate-500 text-center">
          Free and open-source. Built for Windows. BS calendar coverage 1975–2100.
        </p>
        <div className="flex items-center gap-6 text-sm">
          <a href="https://github.com/aayushlbef/Tithify" className="hover:text-white transition-colors focus-ring">GitHub</a>
          <a href="https://github.com/aayushlbef/Tithify/releases/latest" className="hover:text-white transition-colors focus-ring">Releases</a>
          <a href="#install" className="hover:text-white transition-colors focus-ring">Install</a>
          <a href="#faq" className="hover:text-white transition-colors focus-ring">FAQ</a>
          <a href="#donate" className="hover:text-white transition-colors focus-ring">Donate</a>
        </div>
      </div>
    </footer>
  );
}
