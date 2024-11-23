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
}

class MenuViewController: UIViewController, UITableViewDelegate, UITableViewDataSource {

    
    
    weak var delegate: MenuViewControllerDelegate?
    
    enum MenuOptions: String, CaseIterable {
        case reset = "Reset"
        case changeDisk = "Change Disk"
        case saveState = "Save State"
        case loadState = "Load State"
        //case analogMode = "Analog Mode"
        case controllerSetting = "Game Controller"
        
        var imageName: String {
            switch self {
            case .reset:
                return "gobackward"
            case .changeDisk:
                return "opticaldiscdrive"
            case .saveState:
                return "square.and.arrow.up"
            case .loadState:
                return "square.and.arrow.down"
            case .controllerSetting:
                return "gamecontroller"
            //case .analogMode:
            //    return ""
            }
        }
    }

    private let tableView: UITableView = {
        let table = UITableView()
        table.register(UITableViewCell.self, forCellReuseIdentifier: "cell")
        return table
    }()

    //let greyColor = UIColor( red: 33/255.0, green: 33/255.0, blue: 33/255.0, alpha: 1)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        //view.backgroundColor = .white
        view.addSubview(tableView)
        tableView.delegate = self
        tableView.dataSource = self
        //view.backgroundColor = greyColor

/*
        // UITableViewの初期化と設定
        tableView = UITableView(frame: self.view.bounds)
        tableView.delegate = self
        tableView.dataSource = self
        view.addSubview(tableView)
        
        // セルを登録
        tableView.register(UITableViewCell.self, forCellReuseIdentifier: "menuCell")
 */
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        tableView.frame = CGRect(x: 0, y: view.safeAreaInsets.top, width: view.bounds.size.width, height: view.bounds.size.height)
    }
    
    // UITableViewDataSourceメソッド
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return MenuOptions.allCases.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "cell", for: indexPath)
        cell.textLabel?.text = MenuOptions.allCases[indexPath.row].rawValue
        cell.imageView?.image = UIImage(systemName: MenuOptions.allCases[indexPath.row].imageName)
        //cell.textLabel?.textColor = .white
        //cell.imageView?.tintColor = .white
        //cell.backgroundColor = greyColor
        //cell.contentView.backgroundColor = greyColor
        return cell
    }
    
    // UITableViewDelegateメソッド
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        let item = MenuOptions.allCases[indexPath.row]
        delegate?.didSelect(menuItem: item)
        
    }
}
