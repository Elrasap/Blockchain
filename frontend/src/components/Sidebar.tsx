import React from "react";

interface SidebarProps {
  current: string;
  onChange: (key: string) => void;
}

const items = [
  { key: "blocks", label: "Block Explorer" },
  { key: "dnd", label: "DnD Combat Viewer" },
  { key: "peers", label: "Peer Monitor" }
];

export const Sidebar: React.FC<SidebarProps> = ({ current, onChange }) => {
  return (
    <aside className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col">
      <div className="p-4 text-xl font-bold tracking-wide">
        DnD Chain
      </div>
      <nav className="flex-1">
        {items.map((item) => (
          <button
            key={item.key}
            onClick={() => onChange(item.key)}
            className={`w-full text-left px-4 py-2 text-sm hover:bg-slate-800 ${
              current === item.key ? "bg-slate-800 font-semibold" : ""
            }`}
          >
            {item.label}
          </button>
        ))}
      </nav>
    </aside>
  );
};

