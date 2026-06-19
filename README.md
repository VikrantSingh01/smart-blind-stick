# Smart Blind Stick 🦯

An Arduino-powered walking stick that helps visually impaired people detect
obstacles **before** they touch them - and warns them with sound, voice,
vibration, and light.

Built as a school **Science Exhibition** project · Theme: *Future Technology & Robotics*.

## What it does

A normal white stick only finds an obstacle *after* contact. This smart stick
senses what's ahead using an ultrasonic sensor (like a bat's sonar) and warns the
user in several ways at once:

- 🔊 **Beeps** that get faster and higher-pitched as the obstacle gets closer
- 🗣️ **Speaks** "Obstacle ahead" when something is very close
- 📳 **Vibrates** the handle (useful in noisy places)
- 💡 **Lights up** an LED as a visual warning
- 💧 **Detects puddles** / wet floors and gives a distinct alert

## How it works

The ultrasonic sensor sends out sound pulses and measures how long the echo takes
to return:

> **Distance = (Speed of sound × Time) ÷ 2**

The closer the object, the shorter the echo time - and the more urgent the
warning. An Arduino reads the sensor continuously and decides how to alert the
user. It can also stream live distance data to a laptop for a real-time graph.

## What's in this repo

| File | Description |
|---|---|
| `smart_blind_stick.ino` | The Arduino sketch (fully commented for beginners) |
| `1_Motivation_Guide.pdf` | Why we built it - the idea and the inspiration |
| `2_Implementation_Guide.pdf` | Full step-by-step build and code walkthrough |
| `wiring_diagram.pdf` | How to connect every part to the Arduino |
| `display_board.pdf` | The exhibition display-board layout |

## Components (high level)

Arduino UNO R3 · ultrasonic distance sensor · passive buzzer · vibration motor ·
warning LED · voice playback module *(optional)* · water/rain sensor *(optional)* ·
breadboard, jumper wires and USB power.

> Full wiring and assembly steps are in **2_Implementation_Guide.pdf**.

## Highlights

- **Beginner-friendly code** - the sketch is heavily commented so a newcomer can
  follow every line.
- **Non-blocking design** - the stick keeps sensing while it alerts, so it stays
  responsive.
- **Proportional alerts** - the warning grows more urgent as the obstacle nears.
- **Live data** - streams distance readings for a real-time graph and easy
  experiment data.

## Learn more

New to Arduino? This beginner walkthrough explains the board and how to program
it: https://www.youtube.com/watch?v=fJWR7dBuc18

---

*A simple, low-cost assistive-technology project - built to help people walk more
safely and independently.*
