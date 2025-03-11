#!/bin/bash

set -e

WEEKSTART="Mon"
TODAY=$(date -I)
TOMORROW=$(date -j -v +1d -f "%F" "$TODAY" +"%F")

# given a year in $year, generate a "cols" div in $result
gen_year() {
  start=$(date -j -f "%Y" "$year" +%Y-01-01)
  end=$(date -j -v +1y -f "%F" "$start" +%F)
  generate
}

# given dates in $start and $end, generate a "cols" div in $result
generate() {
  local col
  local commit
  local count
  local cur
  local day
  local item
  local items
  local lvl
  local label
  local month
  local next
  local weekday
  local since
  local until

  result=""
  items=""

  # add empty items for each weekday before $start
  cur=$start
  weekday=$(date -j -f "%F" "$cur" +%a)
  while [ "$weekday" != "$WEEKSTART" ]; do
    cur=$(date -j -v -1d -f "%F" "$cur" +%F)
    weekday=$(date -j -f "%F" "$cur" +%a)
    item="      <div class='item'></div>"
    items="${items}${item}"
  done
  WEEKEND=$(date -j -v -1d -f "%F" "$cur" +%a)

  # main loop
  cur=$start
  while [ "$cur" != "$end" ] && [ "$cur" != "$TOMORROW" ]; do
    next=$(date -j -v +1d -f "%F" "$cur" +%F)
    weekday=$(date -j -f "%F" "$cur" +%a)
    day=$(date -j -f "%F" "$cur" +%d)
    month=$(date -j -f "%F" "$cur" +%b)

    # get commit count for current date
    since=$(date -j -f "%F" "$cur" +%FT00:00:00Z)
    until=$(date -j -f "%F" "$next" +%FT00:00:00Z)
    count=$(git shortlog -s --since="$since" --until="$until" | xargs | cut -d " " -f1)
    if [ -z "$count" ]; then
      count=0
    fi

    # pluralize "commit"
    commit="commits"
    if [ "$count" = "1" ]; then
      commit="commit"
    fi

    # determine level based on number of commits
    lvl=0
    if [ "$count" -gt 0 ]; then
      lvl=1
    fi
    if [ "$count" -gt 3 ]; then
      lvl=2
    fi
    if [ "$count" -gt 6 ]; then
      lvl=3
    fi
    if [ "$count" -gt 9 ]; then
      lvl=4
    fi

    # add month label for week containing first day of the month
    if [ "$day" = "01" ]; then
      label="      <div class='label'>${month}</div>"
    fi

    # append day item
    item="      <div class='item lvl${lvl}' title='${cur}: ${count} ${commit}'></div>"
    items="${items}${item}"

    # if this is the last day of the week, roll up items into a "col" and add
    # to $result. $items and $label are reset.
    if [ "$weekday" = "$WEEKEND" ]; then
      col="    <div class='col'>${label}${items}    </div>"
      result="${result}${col}"
      items=""
      label=""
    fi

    cur=$next
  done

  # fill the last week with empty items
  weekday=$(date -j -f "%F" "$cur" +%a)
  while [ "$weekday" != "$WEEKSTART" ]; do
    item="      <div class='item'></div>"
    items="${items}${item}"
    cur=$(date -j -v +1d -f "%F" "$cur" +%F)
    weekday=$(date -j -f "%F" "$cur" +%a)
  done

  # if the loop ended mid-week, accumulate the leftover items
  if [ -n "$items" ]; then
    col="    <div class='col'>${items}    </div>"
    result="${result}${col}"
  fi

  # format the result
  result=$(cat <<-EOF
    <div class='cols'>
      <div class='col labels'>
        <div class='label'>Mon</div>
        <div class='label'></div>
        <div class='label'>Wed</div>
        <div class='label'></div>
        <div class='label'>Fri</div>
        <div class='label'></div>
        <div class='label'></div>
      </div>
${result}
    </div>
EOF
  )
}

# get date of first commit
START=$(git log --date=short --pretty=format:%ad | tail -n 1)
year=$(date -j -f "%F" "$START" +%Y)
end_year=$(date -j -v +1y -f "%F" "$TODAY" +%Y)

# generate a graph for each year
years_result=""
while [ "$year" != "$end_year" ]; do
  gen_year
  years_result=$(cat <<-EOF
<div class='title'>${year}</div>
${result}
${years_result}
EOF
)
  year=$(date -j -v +1y -f "%Y" "$year" +%Y)
done

# format HTML page
result=$(cat <<-EOF
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <title>Commit Activity</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="favicon.svg" />
    <link rel="mask-icon" href="favicon.svg" color="#2D2F38" />
    <link rel="stylesheet" href="style.css" />
    <link rel="stylesheet" href="activity.css" />
  </head>
  <body>
    <main>
      <h1>Commit Activity</h1>
      <p>This is a graph of commit activity in <a href="https://github.com/protestContest/Cassette">Cassette</a>. Last generated ${TODAY}.</p>
      <div class='activity'>
        ${years_result}
      </div>
    </main>
    <footer>
      <hr/>
      <center>
      <p>&copy; 2024 <a href="https://zjm.me">zjm</a></p>
      </center>
    </footer>
  </body>
</html>
EOF
)

echo "$result" > support/site/activity.html
