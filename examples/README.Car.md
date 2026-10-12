# Car
Car is a game for two players. Each player have to drive a car.

## Keys

| Player 1        | Player 2        | Funktion              |
|:----------------|:----------------|:----------------------|
| Curser-Up       | W               | Forward-Acceleration  |
| Curser-Down     | S               | Backward-Acceleration |
| Curser-Left     | A               | Left-Turn             |
| Curser-Right    | D               | Right-Turn            |
| RShift          | LShift          | Slow-Down             |
| Return          | LCtr            | make tunnel           |
| RAlt            | LAlt            | begin/end to tank^    |
| RAlt            | LAlt            | begin/end to repair^^ |

^  only at the tank   station     ^^ only at the repair station

## Rules
- Speed
  - Global Speed Limit
    - Don't violate the speed limit, otherwise the car explodes after 10s driving 10% above the limit.
    - The speed limit changes every minute
  - Speed Status: Smiley
    - If your car's velocity exceeds the speed limit but is still within the 10% tolerance, the face will become `:-|`
    - If the car velocity exceeds the speed limit and tolerance the face will become `:-(`
    - If the car velocity is under the speed limit the face will become: `:-)`
  - Car Speed
    - Don't stand for 30 sec, otherwise the car explodes
    - The car has a maximum velocity
    - If the car is faster than the maximum velocity, it will be reduced and the car loses stability
- Tank
  - The car has to go to the gas station to maintain its fuel
  - If the car tanks up, the player loses money
  - If the tank is empty and the car stands for 10 seconds, it explodes
  - The gas station is marked with a `P`
- Stability
  - If the car intersects with an other thing(e.g. the other car), the stability fall
  - If the stability equals zero, the car explodes
  - The car can go to the repair shop to repair itsself
  - If the car will be repair, the player loses money
  - If the player wants to repair his car super much, that the stability is over the maximum stability,
    the player gets a car, and the stability will set on zero
  - The repair shop is marked with a `R`
- Tunnel
  - One car can makes tunnels for 1 minute
  - If the car makes a tunnel, the player gets money
  - Later, the players cannot drives the cars into the tunnel
- Money
  - If the money is for 10 seconds under zero, the player loses a car
- Mode: Park
  - Speed limit equals 50 km/h
  - The car must go to the parking place
  - At the end of the mode, the car must be on it parking place
- At the start every player has three cars
- A car can brake one second per brake-period
