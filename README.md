# cipereusz

This project is a fully transparent, open-source implementation inspired by GenAI-Bot. 
Unlike the original, which functions as a 'black box' by processing your data on unknown external servers, this version is 100% self-hosted.
This 'spyware-like' behavior is eliminated by running the bot entirely on your own machine, ensuring your server's conversations and data remain completely private.

# Features
- Creating and generating sentences using Markov chains

# Using with docker compose
`docker-compose.yml`
```yaml
services:
  cipereusz-bot:
    image: suchencjusz/cipereusz:latest
    restart: always
    volumes:
      - ./cfg.json:/app/config_cipereusz.json:ro
      - ./cipereusz-bot-data:/cipereusz-bot-data
```
You have to manually create `cfg.json` file and folder `cipereusz-bot-data` before starting the container.

# Configuration
`cfg.json`
```json
{
  "discord_token": "",
  "model_path_n_one": "/cipereusz-bot-data/model_n_one.json",
  "model_path_n_two": "/cipereusz-bot-data/model_n_two.json"
}
```

