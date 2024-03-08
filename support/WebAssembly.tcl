# Template for the hex editor Hex Fiend
# zjm 2024

requires 0 "00 61 73 6D"

global types
set types [dict create]
global funcs
set funcs [dict create]

proc length {start} {
  return [expr [pos] - $start]
}

proc entryfrom {label val start} {
  if {$label != ""} {
    set len [length $start]
    goto $start
    entry $label $val $len
    move $len
  }
  return $val
}

proc leb128 {{label ""}} {
  set start [pos]
  set n 0
  set part [uint8]
  set shift 0

  while {[expr {($part & 0x80) != 0}]} {
    set part [expr ($part & 0x7F) << $shift]
    set n [expr $n | $part]
    set shift [expr $shift + 7]
    set part [uint8]
  }
  set part [expr $part << $shift]
  set n [expr $n | $part]

  entryfrom $label $n $start

  return $n
}

proc valtype {{label ""}} {
  set start [pos]
  set n [uint8]
  if {$n == 0x70} {
    set value func
  } elseif {$n == 0x6F} {
    set value extern
  } elseif {$n == 0x7F} {
    set value i32
  } elseif {$n == 0x7E} {
    set value i64
  } elseif {$n == 0x7D} {
    set value f32
  } elseif {$n == 0x7C} {
    set value f64
  } else {
    set value "?"
  }

  entryfrom $label $value $start
  return $value
}

proc globaltype {{label ""}} {
  set start [pos]
  set gt [valtype]
  set mut [uint8]
  if {$mut == 0} {
    set value "const $gt"
  } else {
    set value "var $gt"
  }

  entryfrom $label $value $start
  return $value
}

proc tabletype {{label ""}} {
  set start [pos]
  set val [valtype]
  set limit [limits]
  set value "$val $limit"

  entryfrom $label $value $start
  return $value
}

proc typeidx {{label ""}} {
  global types
  set start [pos]
  set idx [uint8]
  set value [dict get $types $idx]

  entryfrom $label $value $start
  return $value
}

proc name {{label ""}} {
  set start [pos]
  set len [leb128]
  set value [ascii $len]
  entryfrom $label $value $start
  return $value
}

proc limits {{label ""}} {
  set start [pos]
  set has_max [uint8]
  set min [leb128]
  if {$has_max == 0} {
    set value "$min-∞"
  } else {
    set max [leb128]
    set value "$min-$max"
  }
  entryfrom $label $value $start
  return $value
}

proc expression {{label ""}} {
  set start [pos]
  set b [uint8]
  while {$b != 0x0B} {
    set b [uint8]
  }
  set len [length $start]

  entryfrom $label "\[expr\]" $start
  return "\[expr\]"
}

section -collapsed Header {
  hex 4 Signature
  uint32 Version
}

while {![end]} {
  set section_type [uint8]
  move -1

  if {$section_type == 0x00} {
    section "Custom Section" {
      move 1
      set size [leb128 Size]
      bytes $size Data
    }
  } elseif {$section_type == 0x01} {
    section Types {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        set sig ""
        set start [pos]

        move 1
        set nparams [leb128]
        append sig "("
        for {set j 0} {$j < $nparams} {incr j} {
          append sig [valtype]
          if {$j < [expr $nparams - 1]} {
            append sig ", "
          }
        }
        append sig ") -> ("
        set nresults [leb128]
        for {set j 0} {$j < $nresults} {incr j} {
          append sig [valtype]
          if {$j < [expr $nresults - 1]} {
            append sig ", "
          }
        }
        append sig ")"

        dict set types $i $sig

        entryfrom $i $sig $start
      }
    }
  } elseif {$section_type == 0x02} {
    section Imports {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        section -collapsed $i {
          set start [pos]
          set mod [name]
          set nam [name]

          sectionvalue [entryfrom Name "$mod.$nam" $start]

          set start [pos]
          set type [uint8]
          if {$type == 0x00} {
            set idx [leb128]
            set value [dict get $types $idx]
          } elseif {$type == 0x01} {
            sectionvalue "Table"
            set value [tabletype]
          } elseif {$type == 0x02} {
            sectionvalue "Memory"
            set value [limits]
          } elseif {$type == 0x03} {
            sectionvalue "Global"
            set value [globaltype]
          }

          entryfrom Type $value $start
        }
      }
    }
  } elseif {$section_type == 0x03} {
    section Functions {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        set idx [typeidx $i]
        dict set funcs $i $idx
      }
    }
  } elseif {$section_type == 0x04} {
    section Tables {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        tabletype $i
      }
    }
  } elseif {$section_type == 0x05} {
    section Memories {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        limits $i
      }
    }
  } elseif {$section_type == 0x06} {
    section Globals {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        section -collapsed $i {
          sectionvalue [globaltype Type]
          expression Init
        }
      }
    }
  } elseif {$section_type == 0x07} {
    section Exports {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        section -collapsed $i {
          sectionvalue [name Name]

          set start [pos]
          set type [uint8]
          set idx [leb128]

          if {$type == 0} {
            entryfrom Type "Func $idx" $start
          } elseif {$type == 1} {
            entryfrom Type "Table $idx" $start
          } elseif {$type == 2} {
            entryfrom Type "Mem $idx" $start
          } elseif {$type == 3} {
            entryfrom Type "Global $idx" $start
          }
        }
      }
    }
  } elseif {$section_type == 0x08} {
    section -collapsed Start {
      move 1
      leb128 Size
      set start [pos]
      set idx [leb128]
      sectionvalue "Func $idx"
      entryfrom "Func" $idx $start
    }
  } elseif {$section_type == 0x09} {
    section Elements {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        section -collapsed $i {
          set type [leb128]
          if {$type == 0} {
            entry Mode Active
            entry Table 0
            expression Offset
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                set start [pos]
                set idx [leb128]
                entryfrom $j "Func $idx" $start
              }
            }
          } elseif {$type == 1} {
            entry Mode Passive
            leb128 # elemkind == 0 (func)
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                set start [pos]
                set idx [leb128]
                entryfrom $j "Func $idx" $start
              }
            }
          } elseif {$type == 2} {
            entry Mode Active
            leb128 Table
            expression Offset
            leb128 # elemkind == 0 (func)
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                set start [pos]
                set idx [leb128]
                entryfrom $j "Func $idx" $start
              }
            }
          } elseif {$type == 3} {
            entry Mode Declarative
            leb128 # elemkind == 0 (func)
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                set start [pos]
                set idx [leb128]
                entryfrom $j "Func $idx" $start
              }
            }
          } elseif {$type == 4} {
            entry Mode Active
            entry Table 0
            expression Offset
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                expression $j
              }
            }
          } elseif {$type == 5} {
            entry Mode Passive
            valtype Type
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                expression $j
              }
            }
          } elseif {$type == 6} {
            entry Mode Active
            leb128 Table
            expression Offset
            valtype Type
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                expression $j
              }
            }
          } elseif {$type == 7} {
            entry Mode Declarative
            valtype Type
            section Items {
              set items [leb128]
              for {set j 0} {$j < $items} {incr j} {
                expression $j
              }
            }
          }
        }
      }
    }
  } elseif {$section_type == 0x0A} {
    section Code {
      move 1
      leb128 Size
      set count [leb128 Count]
      for {set i 0} {$i < $count} {incr i} {
        section -collapsed $i {
          puts [pos]
          set size [leb128 Size]
          set start [pos]
          section Locals {
            set numlocals [leb128]
            for {set j 0} {$j < $numlocals} {incr j} {
              set start2 [pos]
              set num [leb128]
              set type [valtype]
              entryfrom $type $num $start2
            }
          }
          puts "$start-[pos]"
          set size [expr $size - [length $start]]
          bytes $size Code
        }
      }
    }
  } elseif {$section_type == 0x0B} {
    section Data {
      move 1
      set size [leb128 Size]
      bytes $size Data
    }
  } elseif {$section_type == 0x0C} {
    section "Data Count" {
      move 1
      set size [leb128 Size]
      bytes $size Data
    }
  } else {
    section "Unknown Section" {
      uint8 -hex Type
      set size [leb128 Size]
      bytes $size Data
    }
  }
}
