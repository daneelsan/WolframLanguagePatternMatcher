(* ::Package:: *)

PacletObject[
  <|
    "Name" -> "DanielS/PatternMatcherVM",
    "Description" -> "A virtual machine for the Wolfram Language pattern matcher",
    "Creator" -> "Daniel Sanchez",
    "License" -> "MIT",
    "PublisherID" -> "DanielS",
    "Version" -> "1.0.0",
    "WolframVersion" -> "14.3+",
    "PrimaryContext" -> "DanielS`PatternMatcherVM`",
    "DocumentationURL" -> "https://resources.wolframcloud.com/PacletRepository/resources",
    "Extensions" -> {
      {
        "Kernel",
        "Root" -> "Kernel",
        "Context" -> {"DanielS`PatternMatcherVM`"},
        "Symbols" -> {
          "DanielS`PatternMatcherVM`CompilePatternToBytecode",
          "DanielS`PatternMatcherVM`CompilePatternToFunction",
          "DanielS`PatternMatcherVM`PatternBytecode",
          "DanielS`PatternMatcherVM`PatternMatcherVirtualMachine",
          "DanielS`PatternMatcherVM`CreatePatternMatcherVirtualMachine",
          "DanielS`PatternMatcherVM`ResetPatternMatcherVirtualMachine",
          "DanielS`PatternMatcherVM`PatternMatcherMatch",
          "DanielS`PatternMatcherVM`PatternMatcherStep"
        }
      },
      {
        "Documentation",
        "Root" -> "Documentation",
        "Language" -> "English"
      }
    }
  |>
]
