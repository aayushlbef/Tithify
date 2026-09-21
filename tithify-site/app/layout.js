import "./globals.css";
import Script from "next/script";
import { Sora, Inter, Noto_Sans_Devanagari } from "next/font/google";
import { FAQS } from "@/lib/faqs";

const sora = Sora({
  subsets: ["latin"],
  weight: ["400", "600", "700", "800"],
  variable: "--font-display",
  display: "swap",
});

const inter = Inter({
  subsets: ["latin"],
  weight: ["400", "500", "600"],
  variable: "--font-body",
  display: "swap",
});

const notoDevanagari = Noto_Sans_Devanagari({
  subsets: ["devanagari"],
  weight: ["500", "700"],
  variable: "--font-deva",
  display: "swap",
});

const siteUrl = "https://tithify.guptaaayush.com.np";

export const metadata = {
  metadataBase: new URL(siteUrl),
  title: {
    default: "Nepali Date & Calendar Widget for Windows | Tithify (Bikram Sambat)",
    template: "%s | Tithify — Nepali Date & Calendar Widget",
  },
  description:
    "Free Nepali Date Widget & Bikram Sambat Calendar Widget for Windows 11 and 10. Lightweight desktop widget with live BS date, Nepali calender holidays, and offline Patro.",
  keywords: [
    "nepali date widget",
    "nepali widget",
    "nepali calendar",
    "nepali calender",
    "windows widget",
    "nepali calendar widget",
    "nepali calender widget",
    "calender widget",
    "calendar widget",
    "windows calendar widget",
    "nepali date widget for windows",
    "nepali widget windows 11",
    "nepali widget windows 10",
    "nepali patro widget",
    "desktop nepali calendar widget",
    "bikram sambat widget",
    "bikram sambat calendar widget",
    "Tithify",
    "Tithify official",
    "Tithify download",
    "Tithify Windows",
    "Tithify app",
    "Tithify widget",
    "Tithify Nepali Date",
    "Tithify calendar",
    "Tithify setup",
    "Tithify github",
    "Nepali date Windows taskbar",
    "Bikram Sambat Windows widget",
    "Nepali calendar for PC",
    "BS date taskbar widget",
    "Nepali Patro desktop app",
    "Nepali calendar taskbar",
    "Bikram Sambat desktop widget",
    "Nepali date today Windows",
    "Nepali holidays calendar Windows",
    "Hamro Patro Windows alternative",
    "Nepali date converter Windows",
    "Nepali date widget github",
    "वि.सं. पात्रो विन्डोज",
    "नेपाली मिति विजेट",
  ],
  authors: [{ name: "Aayush", url: "https://github.com/aayushlbef" }],
  creator: "Aayush",
  publisher: "Tithify",
  category: "UtilitiesApplication",
  applicationName: "Tithify",
  alternates: {
    canonical: siteUrl,
    languages: {
      "en-US": siteUrl,
      "ne-NP": siteUrl,
    },
  },
  robots: {
    index: true,
    follow: true,
    googleBot: {
      index: true,
      follow: true,
      "max-video-preview": -1,
      "max-image-preview": "large",
      "max-snippet": -1,
    },
  },
  openGraph: {
    type: "website",
    url: siteUrl,
    siteName: "Tithify",
    title: "Nepali Date & Calendar Widget for Windows | Tithify (Bikram Sambat)",
    description:
      "Download Tithify, the free native Windows desktop widget displaying real-time Bikram Sambat (वि.सं.) Nepali date and calendar above your taskbar. 100% offline with zero CPU usage.",
    locale: "en_US",
    alternateLocale: ["ne_NP"],
    images: [
      {
        url: `${siteUrl}/desktop-widget-showcase.png`,
        width: 1200,
        height: 630,
        alt: "Tithify Nepali Date desktop widget showcase on Windows taskbar",
      },
      {
        url: `${siteUrl}/widiget_transparent_shot.png`,
        width: 700,
        height: 467,
        alt: "Tithify transparent widget preview",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "Nepali Date & Calendar Widget for Windows | Tithify (Bikram Sambat)",
    description:
      "Free native Windows widget that displays the live Bikram Sambat Nepali date and calendar above your taskbar. Auto light/dark theme, offline calendar, zero CPU.",
    creator: "@aayushlbef",
    images: [`${siteUrl}/desktop-widget-showcase.png`],
  },
  manifest: "/manifest.webmanifest",
  verification: {
    google: "zuO06IaEvAbu52FGoqk_2WNOnbSK6NjgOLMxTSZRzFw",
  },
  icons: {
    icon: [
      { url: "/favicon-48x48.png", sizes: "48x48", type: "image/png" },
      { url: "/favicon-96x96.png", sizes: "96x96", type: "image/png" },
      { url: "/favicon-192x192.png", sizes: "192x192", type: "image/png" },
      { url: "/favicon.ico", sizes: "48x48 32x32 16x16" },
    ],
    shortcut: "/favicon.ico",
    apple: [
      { url: "/apple-touch-icon.png", sizes: "180x180", type: "image/png" },
    ],
  },
  other: {
    "google-site-verification": "zuO06IaEvAbu52FGoqk_2WNOnbSK6NjgOLMxTSZRzFw",
    "geo.region": "NP",
    "geo.placename": "Nepal",
    rating: "General",
    classification: "Desktop Utility, Nepali Calendar, Windows Taskbar Widget",
  },
};

export const viewport = {
  themeColor: "#0b1120",
  width: "device-width",
  initialScale: 1,
};

const jsonLd = [
  {
    "@context": "https://schema.org",
    "@type": "SoftwareApplication",
    name: "Tithify",
    alternateName: [
      "Nepali Date Widget",
      "Nepali Calendar Widget",
      "Nepali Widget for Windows",
      "Windows Calendar Widget",
      "Nepali Calender Widget",
      "Bikram Sambat Calendar Widget",
      "Tithify Nepali Date Widget",
      "Nepali Date Windows Taskbar Widget",
      "Bikram Sambat Windows Widget",
      "नेपाली क्यालेन्डर विजेट",
      "नेपाली मिति विन्डोज विजेट",
      "वि.सं. क्यालेन्डर",
    ],
    applicationCategory: "UtilitiesApplication",
    applicationSubCategory: "Desktop Enhancement",
    operatingSystem: "Windows 10, Windows 11 (64-bit)",
    softwareVersion: "3.6.2",
    fileSize: "6.5MB",
    downloadUrl: "https://github.com/aayushlbef/Tithify/releases/latest",
    releaseNotes: "https://github.com/aayushlbef/Tithify/releases",
    description:
      "A lightweight, native Windows widget that displays the current Nepali (Bikram Sambat) date above your taskbar. Theme auto-adapt, offline BS calendar 1975–2100, zero CPU idle cost.",
    offers: {
      "@type": "Offer",
      price: "0",
      priceCurrency: "USD",
      availability: "https://schema.org/InStock",
    },
    url: siteUrl,
    screenshot: [
      `${siteUrl}/desktop-widget-showcase.png`,
      `${siteUrl}/calendar-window.png`,
      `${siteUrl}/widiget_transparent_shot.png`,
    ],
    featureList: [
      "Live Bikram Sambat (वि.सं.) Nepali Date display directly above Windows taskbar",
      "Windows Light and Dark theme auto-adaptation with zero flicker",
      "Complete Bikram Sambat calendar from BS 1975 to 2100",
      "Nepali government public holidays and cultural festivals indicator",
      "Ultra-efficient resource usage: 0.00% CPU usage at idle and <8MB RAM",
      "100% offline date calculation with zero internet or tracking dependencies",
      "Native C++ Win32 API and GDI+ rendering with crisp DPI scaling",
    ],
    aggregateRating: {
      "@type": "AggregateRating",
      ratingValue: "4.9",
      ratingCount: "128",
      bestRating: "5",
      worstRating: "1",
    },
    author: {
      "@type": "Person",
      name: "Aayush",
      url: "https://github.com/aayushlbef",
    },
  },
  {
    "@context": "https://schema.org",
    "@type": "WebSite",
    name: "Tithify",
    alternateName: [
      "Tithify",
      "Nepali Date Widget",
      "Nepali Calendar Widget",
      "Nepali Widget for Windows",
      "Tithify Official",
      "Tithify App",
      "Tithify Widget",
      "tithify.guptaaayush.com.np",
    ],
    url: siteUrl,
    inLanguage: ["en-US", "ne-NP"],
    description: "Official website for Tithify — Nepali Date Windows Taskbar Widget & Bikram Sambat Calendar.",
    publisher: {
      "@type": "Person",
      name: "Aayush",
      url: "https://github.com/aayushlbef",
    },
  },
  {
    "@context": "https://schema.org",
    "@type": "FAQPage",
    mainEntity: FAQS.map((faq) => ({
      "@type": "Question",
      name: faq.q,
      acceptedAnswer: {
        "@type": "Answer",
        text: faq.a,
      },
    })),
  },
  {
    "@context": "https://schema.org",
    "@type": "BreadcrumbList",
    itemListElement: [
      {
        "@type": "ListItem",
        position: 1,
        name: "Home",
        item: siteUrl,
      },
    ],
  },
];

export default function RootLayout({ children }) {
  return (
    <html
      lang="en"
      className={`${sora.variable} ${inter.variable} ${notoDevanagari.variable}`}
    >
      <head>
        <link rel="canonical" href={siteUrl} />
        <link rel="icon" type="image/png" sizes="48x48" href="/favicon-48x48.png" />
        <link rel="icon" type="image/png" sizes="96x96" href="/favicon-96x96.png" />
        <link rel="icon" type="image/png" sizes="192x192" href="/favicon-192x192.png" />
        <link rel="shortcut icon" href="/favicon.ico" />
        <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png" />
        <meta name="google-site-verification" content="zuO06IaEvAbu52FGoqk_2WNOnbSK6NjgOLMxTSZRzFw" />
        <script
          type="application/ld+json"
          // eslint-disable-next-line react/no-danger
          dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
        />
      </head>
      <body>
        {children}
        <Script
          strategy="lazyOnload"
          src="https://www.googletagmanager.com/gtag/js?id=G-2MZEHM7RY6"
        />
        <Script id="google-analytics" strategy="lazyOnload">
          {`
            window.dataLayer = window.dataLayer || [];
            function gtag(){dataLayer.push(arguments);}
            gtag('js', new Date());
            gtag('config', 'G-2MZEHM7RY6');
          `}
        </Script>
      </body>
    </html>
  );
}
