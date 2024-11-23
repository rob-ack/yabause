//
//  MenuViewController.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/20.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation

protocol MenuViewControllerDelegate: AnyObject {
    func didSelect(menuItem: MenuViewController.MenuOptions)
    func didChangeAnalogMode(to: Bool)
}

class SwitchTableViewCell: UITableViewCell {

    let toggleSwitch: UISwitch = {
        let toggleSwitch = UISwitch()
        toggleSwitch.translatesAutoresizingMaskIntoConstraints = false
        return toggleSwitch
    }()

    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupLayout()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupLayout()
    }

    private func setupLayout() {
        contentView.addSubview(toggleSwitch)
        
        // スイッチを左端に配置
        NSLayoutConstraint.activate([
            toggleSwitch.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            toggleSwitch.centerYAnchor.constraint(equalTo: contentView.centerYAnchor)
        ])
    }
}


class MenuViewController: UIViewController, UITableViewDelegate, UITableViewDataSource {

    weak var delegate: MenuViewControllerDelegate?

    enum MenuOptions: String, CaseIterable {
        case exit = "Exit"
        case reset = "Reset"
        case changeDisk = "Change Disk"
        case backupManager = "Backup Manager"
        case saveState = "Save State"
        case loadState = "Load State"
        case analogMode = "Analog Mode"
        case controllerSetting = "Game Controller"

        var imageName: String {
            switch self {
            case .exit:
                return "power"
            case .reset:
                return "gobackward"
            case .changeDisk:
                return "opticaldiscdrive"
            case .saveState:
                return "square.and.arrow.up"
            case .loadState:
                return "square.and.arrow.down"
            case .analogMode:
                return "switch.2"
            case .controllerSetting:
                return "gamecontroller"
            case .backupManager:
                return "lock.square"

            }
        }

        var localizedTitle: String {
            switch self {
            case .exit:
                return NSLocalizedString("Exit", comment: "Exit the application")
            case .reset:
                return NSLocalizedString("Reset", comment: "Reset the game")
            case .changeDisk:
                return NSLocalizedString("Change Disk", comment: "Change the game disk")
            case .saveState:
                return NSLocalizedString("Save State", comment: "Save the current state of the game")
            case .loadState:
                return NSLocalizedString("Load State", comment: "Load a previously saved game state")
            case .analogMode:
                return NSLocalizedString("Analog Mode", comment: "Switch to analog mode")
            case .controllerSetting:
                return NSLocalizedString("Game Controller", comment: "Game controller settings")
            case .backupManager:
                return NSLocalizedString("Backup Manager", comment: "Backup Manager settings")

            }
        }
    }

    private let tableView: UITableView = {
        let table = UITableView()
        table.register(UITableViewCell.self, forCellReuseIdentifier: "cell")
        table.register(UITableViewCell.self, forCellReuseIdentifier: "switchCell")
        return table
    }()

    override func viewDidLoad() {
        super.viewDidLoad()
        view.addSubview(tableView)
        tableView.delegate = self
        tableView.dataSource = self
    }

    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        tableView.frame = CGRect(x: 0, y: view.safeAreaInsets.top, width: 240, height: view.bounds.size.height)
    }

    // UITableViewDataSourceメソッド
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return MenuOptions.allCases.count
    }

    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let option = MenuOptions.allCases[indexPath.row]

        if option == .analogMode {
            let cell = tableView.dequeueReusableCell(withIdentifier: "switchCell", for: indexPath)
            cell.textLabel?.text = option.localizedTitle
            let toggleSwitch = UISwitch()
            cell.accessoryView = toggleSwitch
            let path = SettingsViewController.getSettingFilname()
            if let dic = NSDictionary(contentsOfFile: path) {
                toggleSwitch.isOn = (dic["analog mode"] as? Bool) ?? false
            }
            toggleSwitch.addTarget(self, action: #selector(didChangeAnalogModeSwitch(_:)), for: .valueChanged)
            return cell
        } else {
            let cell = tableView.dequeueReusableCell(withIdentifier: "cell", for: indexPath)
            cell.textLabel?.text = option.localizedTitle
            cell.imageView?.image = UIImage(systemName: option.imageName)
            return cell
        }
    }

    // トグルスイッチが変更されたときに呼ばれるメソッド
    @objc func didChangeAnalogModeSwitch(_ sender: UISwitch) {
        // Analog Modeの状態が変更されたときの処理をここに記述します
        let isAnalogModeOn = sender.isOn
        delegate?.didChangeAnalogMode(to: isAnalogModeOn)
    }

    // UITableViewDelegateメソッド
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        let item = MenuOptions.allCases[indexPath.row]
        delegate?.didSelect(menuItem: item)
    }
}
