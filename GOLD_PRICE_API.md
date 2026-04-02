# Real-Time Gold Price Configuration

The gold price fetcher supports multiple API options:

## Option 1: Metals.live API (Currently Configured - FREE, No Key Required)
- **URL**: https://api.metals.live/v1/spot/gold
- **Free**: Yes, no API key needed
- **Rate Limit**: No strict limit
- **Response Format**: JSON with `price`, `ch` (change), `chp` (change percent)

## Option 2: GoldAPI.io (Better Data, Requires Free Account)
1. Sign up at https://www.goldapi.io/
2. Get your free API key (50 requests/day on free tier)
3. Open `main/gold_price.c`
4. Replace line 13: `#define API_KEY "goldapi-YOUR_API_KEY_HERE"` with your key
5. Change line 51: `.url = ALT_API_URL` to `.url = GOLD_API_URL`
6. Uncomment line 57: `esp_http_client_set_header(client, "x-access-token", API_KEY);`

## Option 3: Other APIs
You can integrate other gold price APIs by modifying `gold_price.c`:
- **MetalPriceAPI.com** - Free tier available
- **CurrencyAPI.com** - Supports gold prices
- **Twelve Data** - Financial data API

## Update Frequency
The default update interval is 60 seconds (60,000 ms) as defined in `main/clock_config.h`:
```c
#define GOLD_UPDATE_INTERVAL_MS  60000     // Update gold price every 60 seconds
```

**Note**: Be mindful of API rate limits. For free tiers, updating every 1-2 minutes is recommended.

## WiFi Configuration
Make sure to configure your WiFi credentials in `main/wifi_config.h`:
```c
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
```

The gold price fetcher will automatically skip updates if WiFi is not connected.
