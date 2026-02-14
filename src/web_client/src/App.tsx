import { useState } from 'react'

function App() {
    const [count, setCount] = useState(0)

    return (
        <div style={{ padding: 20, font: 'sans-serif', color: 'white', background: '#222' }}>
            <h1>FlowZone</h1>
            <p>Web Client v0.0.1</p>
            <button onClick={() => setCount((count) => count + 1)}>
                count is {count}
            </button>
        </div>
    )
}

export default App
