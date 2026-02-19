# FlowZone Testing Guide

## 1. Interactive Web Client (Frontend)

The frontend is a React application that mimics the FlowZone UI. It currently uses a **Mock State Provider** to simulate backend behavior (playing, pausing, state updates), allowing you to test the UI flow without the C++ engine running.

**To run the Web Client:**

1.  Open a terminal.
2.  Navigate to the web client directory:
    ```bash
    cd src/web_client
    ```
3.  Start the development server:
    ```bash
    npm run dev
    ```
4.  Open the link provided (usually `http://localhost:5173`) in your browser.

## 2. Audio Engine Verification (Backend)

The C++ Audio Engine core (`TransportService`, `StateBroadcaster`, `SessionStateManager`) has been implemented as a static library and verified using unit tests. There is no standalone GUI app for the engine yet, as it is designed to be headless or a plugin.

**To verify the Engine logic:**

1.  Open a terminal in the project root.
2.  Run the test executable:
    ```bash
    ./build/engine_tests
    ```
3.  You should see output indicating all tests passed (e.g., "All tests passed (30 assertions in 4 test cases)").
