#!/bin/bash
set -e

echo "🛠 Compiling with coverage flags..."
make clean
make CFLAGS="-Wall -Wextra -std=c99 -fprofile-arcs -ftest-coverage -g"

echo "🚀 Starting drinks_bar in background..."
./drinks_bar -T 5555 -U 7777 -f file.txt -t 60 -c 100 -o 100 -h 100 > server.log 2>&1 &
SERVER_PID=$!

# חכה שהשרת יהיה מוכן
echo "⏳ Waiting for server to be ready..."
sleep 3


echo "👥 Starting atom_supplier in background..."
(
  echo "ADD CARBON 100"
  echo "ADD OXYGEN 100" 
  echo "ADD HYDROGEN 100"
  sleep 8  # זמן הרצה לפני סיום
) | ./atom_supplier -h 127.0.0.1 -p 5555 > atom.log 2>&1 &
ATOM_PID=$!

echo "👥 Starting molecule_requester in background..."
(
  echo "DELIVER WATER 3"
  echo "DELIVER CARBON DIOXIDE 3"
  echo "DELIVER ALCOHOL 3"
  echo "DELIVER GLUCOSE 3"
  sleep 8
) | ./molecule_requester -h 127.0.0.1 -p 7777 > molecule.log 2>&1 &
MOLECULE_PID=$!

echo "⏳ Letting clients run for a few seconds..."
sleep 10

echo "🛑 Killing clients (simulating Ctrl+C)..."
kill $ATOM_PID 2>/dev/null || true
kill $MOLECULE_PID 2>/dev/null || true
wait $ATOM_PID 2>/dev/null || true
wait $MOLECULE_PID 2>/dev/null || true

# echo "📡 Sending GEN commands to server..."
# sleep 1  # תן לשרת זמן לעבד את סיום הלקוחות
# echo "GEN VODKA" | socat - TCP:127.0.0.1:5555 || echo "⚠️ Failed to send GEN VODKA"
# sleep 1
# echo "GEN SOFT DRINK" | socat - TCP:127.0.0.1:5555 || echo "⚠️ Failed to send GEN SOFT DRINK"
# sleep 1
# echo "GEN CHAMPAGNE" | socat - TCP:127.0.0.1:5555 || echo "⚠️ Failed to send GEN CHAMPAGNE"
# sleep 1

echo "🔚 Sending exit signal to server..."
# שלח סיגנל סיום לשרת (SIGTERM)
kill -TERM $SERVER_PID 2>/dev/null || true

echo "⏳ Waiting for server to finish..."
wait $SERVER_PID 2>/dev/null || true

echo "📊 Generating gcov reports..."
gcov *.c > /dev/null || echo "⚠️ Some gcov files might be missing"

echo "📈 Generating lcov HTML report..."
lcov --capture --directory . --output-file coverage.info --quiet || echo "⚠️ lcov capture failed"
genhtml coverage.info --output-directory coverage_html --quiet || echo "⚠️ genhtml failed"

echo "✅ Coverage analysis complete!"
echo "📋 Coverage report available at: coverage_html/index.html"
echo "📄 Log files: server.log, atom.log, molecule.log"