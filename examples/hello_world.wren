System.print("不死鳥戦隊" + "フェザーマンR")

var 日本語 = "こんにちは"
System.print(日本語 + ", " + 日本語.length)

class Wren {
  flyTo(city) {
    System.print("Flying to %(city)")
  }
}

var adjectives = Fiber.new {
  ["small", "clean", "fast"].each {|word| Fiber.yield(word) }
}

while (!adjectives.isDone) System.print(adjectives.call())