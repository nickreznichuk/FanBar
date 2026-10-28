import AppKit

final class AppDelegate: NSObject, NSApplicationDelegate {
    let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    var timer: Timer?

    func applicationDidFinishLaunching(_ notification: Notification) {
        _ = smc_open()
        item.button?.image = NSImage(systemSymbolName: "fan", accessibilityDescription: "Вентилятори")
        item.button?.imagePosition = .imageLeading
        update()
        timer = Timer.scheduledTimer(withTimeInterval: 3, repeats: true) { [weak self] _ in self?.update() }
    }

    func update() {
        let fans = (0..<max(0, Int(fan_count()))).map { i in
            (actual: fan_value(Int32(i), "Ac"), target: fan_value(Int32(i), "Tg"),
             min: fan_value(Int32(i), "Mn"), max: fan_value(Int32(i), "Mx"))
        }
        let temp = max_die_temp()
        let state = ProcessInfo.processInfo.thermalState
        let rpm = fans.map(\.actual).max() ?? 0

        let color: NSColor
        if temp >= 90 || state == .serious || state == .critical { color = .systemRed }
        else if temp >= 80 || state == .fair { color = .systemOrange }
        else { color = .labelColor }

        let tempText = temp > 0 ? String(format: "%.0f°", temp) : "—°"
        let rpmText = rpm > 0 ? String(format: "%.0f", rpm) : "off"
        item.button?.attributedTitle = NSAttributedString(
            string: " \(tempText) \(rpmText)",
            attributes: [.foregroundColor: color,
                         .font: NSFont.monospacedDigitSystemFont(ofSize: 12, weight: .regular)])

        let menu = NSMenu()
        for (i, f) in fans.enumerated() {
            let line = String(format: "Вентилятор %d: %.0f об/хв (ціль %.0f, діапазон %.0f–%.0f)",
                              i + 1, f.actual, f.target, f.min, f.max)
            menu.addItem(withTitle: line, action: nil, keyEquivalent: "")
        }
        if fans.isEmpty { menu.addItem(withTitle: "Вентилятори: немає даних", action: nil, keyEquivalent: "") }
        menu.addItem(withTitle: temp > 0 ? String(format: "Температура процесора: %.1f°C", temp)
                                         : "Температура процесора: немає даних",
                     action: nil, keyEquivalent: "")
        menu.addItem(withTitle: "Тепловий стан: \(describe(state))", action: nil, keyEquivalent: "")
        menu.addItem(.separator())
        menu.addItem(withTitle: "Вийти", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        item.menu = menu
    }

    func describe(_ s: ProcessInfo.ThermalState) -> String {
        switch s {
        case .nominal: return "норма"
        case .fair: return "підвищений"
        case .serious: return "серйозний"
        case .critical: return "критичний"
        @unknown default: return "невідомо"
        }
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.accessory)
app.run()
