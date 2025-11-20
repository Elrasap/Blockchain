import React, { useState } from "react";
import { Sidebar } from "./components/Sidebar";
import { BlockExplorer } from "./components/BlockExplorer";
import { DndCombatViewer } from "./components/DndCombatViewer";
import { PeerMonitor } from "./components/PeerMonitor";

const App: React.FC = () => {
  const [current, setCurrent] = useState("blocks");

  const renderView = () => {
    switch (current) {
      case "blocks": return <BlockExplorer />;
      case "dnd":    return <DndCombatViewer />;
      case "peers":  return <PeerMonitor />;
      default:       return <BlockExplorer />;
    }
  };

  return (
    <div className="h-screen flex">
      <Sidebar current={current} onChange={setCurrent} />
      <main className="flex-1 flex flex-col">
        <header className="border-b border-slate-800 px-6 py-3 flex items-center justify-between bg-slate-950/80">
          <div className="text-sm text-slate-400">
            DnD Blockchain Node Dashboard
          </div>
        </header>
        <section className="flex-1 p-6 overflow-y-auto">
          {renderView()}
        </section>
      </main>
    </div>
  );
};

export default App;

