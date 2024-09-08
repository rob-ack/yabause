import UIKit

struct BackupItem {
    var index: Int32 = 0
     var filename: String?
     var comment: String?
     var language: Int32 = 0
     var savedate: String?
     var datasize: Int32 = 0
     var blocksize: Int32 = 0
     var url: String = ""
     var key: String?
    
    // DATE_PATTERN equivalent
    static let dateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy/MM/dd HH:mm"
        return formatter
    }()
}

struct BackupDevice {
    var name: String
    var id: Int
    
    static let DEVICE_CLOUD = 9999 // Assuming this is the correct value for cloud device
}

class BackupFileListViewController: UIViewController {
    
    private var backupManagerTabBarController: BackupManagerTabBarController!
    private var backupDevices: [BackupDevice] = []
    
    var completionHandler: ((_ selectedBackup: BackupItem?) -> Void)?
    
    init(completionHandler: ((_ selectedBackup: BackupItem?) -> Void)? = nil) {
        self.completionHandler = completionHandler
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        fetchDeviceList()
        setupBackupManagerTabBarController()
        setupNavigationBar()
    }
    
    private func setupBackupManagerTabBarController() {
        backupManagerTabBarController = BackupManagerTabBarController(backupDevices: backupDevices)
        addChild(backupManagerTabBarController)
        view.addSubview(backupManagerTabBarController.view)
        backupManagerTabBarController.view.frame = view.bounds
        backupManagerTabBarController.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        backupManagerTabBarController.didMove(toParent: self)
    }
    
    private func setupNavigationBar() {
        let closeButton = UIBarButtonItem(title: NSLocalizedString("Close", comment: "Close Dialoig"), style: .plain, target: self, action: #selector(closeButtonTapped))
        navigationItem.leftBarButtonItem = closeButton
        title = NSLocalizedString("Backup Manager", comment: "Backup Manager Settings")
    }
    
    @objc private func closeButtonTapped() {
        dismiss(animated: true) {
            self.completionHandler?(nil)
        }
    }
    
    
    private func fetchDeviceList() {
        guard let jsonStr = YSGetBackupDevicelist()else {
            print("Failed to get device list")
            return
        }
        do {
               if let jsonData = jsonStr.data(using: .utf8) {
                   if let json = try JSONSerialization.jsonObject(with: jsonData, options: []) as? [String: Any] {
                       if let devices = json["devices"] as? [[String: Any]] {
                           for device in devices {
                               if let name = device["name"] as? String,
                                  let id = device["id"] as? Int {
                                   let backupDevice = BackupDevice(name: name, id: id)
                                   backupDevices.append(backupDevice)
                               }
                           }
                       }
                   }
               }
               
               // Add cloud device
               //let cloudDevice = BackupDevice(name: "cloud", id: BackupDevice.DEVICE_CLOUD)
               //backupDevices.append(cloudDevice)
               
           } catch {
               print("Failed to parse JSON: \(error.localizedDescription)")
           }
           
           if backupDevices.isEmpty {
               print("Can't find any devices")
           }
           
           // Here you would typically update your UI, e.g., reload a table view
           // For this example, we'll just print the devices
           for device in backupDevices {
               print("Device: \(device.name), ID: \(device.id)")
           }
        
        
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        if self.isBeingDismissed {
            completionHandler?(nil)
        }
    }

    @objc private func cancel() {
        completionHandler?(nil)
        dismiss(animated: true) {
            self.completionHandler?(nil)
        }
    }
    
    func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
         completionHandler?(nil)
     }

}



class BackupManagerTabBarController: UITabBarController {
    var backupDevices: [BackupDevice] = []
    var backupList: [BackupDeviceViewController] = []
    
    
    init(backupDevices: [BackupDevice]) {
        self.backupDevices = backupDevices
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupViewControllers()
    }
    
    private func setupViewControllers() {
        var viewControllers: [UIViewController] = []
        
        self.view.backgroundColor = .systemBackground
        
        for (index, device) in backupDevices.enumerated() {
            let deviceVC = BackupDeviceViewController(device: device, deviceIndex: Int32(index)){ selectedBackup in
                    self.showBackupOptions(device: device, for: selectedBackup)
            }
            deviceVC.tabBarItem = UITabBarItem(title: device.name, image: nil, tag: index)
            backupList.append(deviceVC)
            viewControllers.append(deviceVC)
        }
        
        self.viewControllers = viewControllers
    }
    
    private func showBackupOptions(device: BackupDevice, for backupItem: BackupItem) {
        let alertController = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        if( backupDevices.count > 1) {
            if( device.id == 0 ){
                let message = String( format: NSLocalizedString("Copy to [%@]", comment: "copy a backup file to external device") ,backupDevices[1].name)
                alertController.addAction(UIAlertAction(title: message, style: .default) { _ in
                    YSCopy( Int32(self.backupDevices[1].id), backupItem.index )
                })
                
            }else{
                alertController.addAction(UIAlertAction(title: NSLocalizedString("Copy to Internal memory", comment: "copy a backup file to internal memory"), style: .default) { _ in
                    YSCopy( Int32(self.backupDevices[0].id), backupItem.index )
                    //self.backupList[0].updateSaveList()
                })
            }
        }
        
        
        alertController.addAction(UIAlertAction(title: NSLocalizedString("Delete", comment: "Delete file"), style: .destructive) { _ in
            
            let message = String( format: NSLocalizedString("Are you sure you want to delete [%@]", comment: "Confirm deletion message") ,backupItem.filename ?? "")
            
            let confirmAlert = UIAlertController(title: NSLocalizedString("Confirm Deletion", comment: "Confirm deletion alert title"),
                                                  message: message,
                                                  preferredStyle: .alert)
             
             confirmAlert.addAction(UIAlertAction(title: NSLocalizedString("Cancel", comment: "Cancel deletion"), style: .cancel, handler: nil))
             
             confirmAlert.addAction(UIAlertAction(title: NSLocalizedString("Delete", comment: "Confirm deletion"), style: .destructive) { _ in
                 // Proceed with deletion
                 YSDeleteBackupFile(backupItem.index)
                 if device.id == 0 {
                     self.backupList[0].updateSaveList()
                 } else {
                     self.backupList[1].updateSaveList()
                 }
             })
            
            self.present(confirmAlert, animated: true, completion: nil)
            
        })
        
        alertController.addAction(UIAlertAction(title: NSLocalizedString("Cancel",comment: "cancel"), style: .cancel, handler: nil))
        
        // Configure popover presentation controller for iPad
        if let popoverController = alertController.popoverPresentationController {
            popoverController.sourceView = view
            popoverController.sourceRect = CGRect(x: view.bounds.midX, y: view.bounds.midY, width: 0, height: 0)
            popoverController.permittedArrowDirections = []
        }
        
        present(alertController, animated: true)
    }
}



class BackupDeviceViewController: UIViewController, UITableViewDataSource, UITableViewDelegate {
    
    private var tableView: UITableView!
    
    
    private var device: BackupDevice
    private var deviceIndex: Int32
    
    private var backupItems: [BackupItem] = []
    private var currentPage: Int = 0
    private var totalSize: Int32 = 0
    private var freeSize: Int32 = 0
    
    private var sumLabel: UILabel!
    
    var showBackupOption: ((_ selectedBackup: BackupItem) -> Void)?
    
    
    init(device: BackupDevice, deviceIndex: Int32, showBackupOption: ((_ selectedBackup: BackupItem) -> Void)? = nil) {
        self.device = device
        self.deviceIndex = deviceIndex
        self.showBackupOption = showBackupOption
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupViews()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        updateSaveList()
    }

    
    private func setupViews() {
        // Setup tableView
        tableView = UITableView(frame: .zero, style: .plain)
        tableView.translatesAutoresizingMaskIntoConstraints = false
        tableView.dataSource = self
        tableView.delegate = self
        tableView.backgroundColor = .systemBackground
        tableView.register(BackupItemCell.self, forCellReuseIdentifier: "BackupItemCell")
        view.addSubview(tableView)
        
        // Setup sumLabel
        sumLabel = UILabel()
        sumLabel.translatesAutoresizingMaskIntoConstraints = false
        sumLabel.textAlignment = .center
        sumLabel.font = UIFont.systemFont(ofSize: 14)
        sumLabel.textColor = .secondaryLabel
        sumLabel.backgroundColor = .systemBackground  // システムの背景色を使用
        view.addSubview(sumLabel)
        
        // Setup constraints
        NSLayoutConstraint.activate([
            tableView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            tableView.bottomAnchor.constraint(equalTo: sumLabel.topAnchor),
            
            sumLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            sumLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            sumLabel.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor),
            sumLabel.heightAnchor.constraint(equalToConstant: 44)  // 適切な高さを設定
        ])
    }
    
    
    
    private func loadSampleData() {
        backupItems = [
            BackupItem(index: 0, filename: "Backup_2024-03-15.zip", comment: "Latest backup", language: 1, savedate: "2024/03/15 14:30", datasize: 10240, blocksize: 4096, url: "https://example.com/backup1"),
            BackupItem(index: 1, filename: "Backup_2024-03-08.zip", comment: "Weekly backup", language: 1, savedate: "2024/03/08 12:00", datasize: 8192, blocksize: 4096, url: "https://example.com/backup2"),
            BackupItem(index: 2, filename: "Backup_2024-03-01.zip", comment: "Monthly backup", language: 1, savedate: "2024/03/01 00:00", datasize: 15360, blocksize: 4096, url: "https://example.com/backup3")
        ]
    }
    
    
    private func setupSumLabel() {
        sumLabel = UILabel()
        sumLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(sumLabel)
        
        NSLayoutConstraint.activate([
            sumLabel.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -8),
            sumLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            sumLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16)
        ])
        
        updateSumLabel()
    }
    
    private func updateSumLabel() {
        let numberFormatter = NumberFormatter()
        numberFormatter.numberStyle = .decimal
        numberFormatter.groupingSeparator = ","
        numberFormatter.groupingSize = 3
        
        let useszie = totalSize - freeSize
            
        let formattedFreeSize = numberFormatter.string(from: NSNumber(value: useszie)) ?? "\(useszie)"
        let formattedTotalSize = numberFormatter.string(from: NSNumber(value: totalSize)) ?? "\(totalSize)"
            
        // Localization implemented
        let localizedFormat = NSLocalizedString("%@ Byte used / %@ Byte total", comment: "Format string for storage info")
        sumLabel.text = String(format: localizedFormat, formattedFreeSize, formattedTotalSize)
      
    }
    
    func updateSaveList() {
        
        let device =  self.deviceIndex
        
        guard let jsonStr = YSGetBackupFilelist(device) else {
            print("Failed to get file list")
            return
        }
        
        backupItems.removeAll()
        
        do {
            if let jsonData = jsonStr.data(using: .utf8),
               let json = try JSONSerialization.jsonObject(with: jsonData, options: []) as? [String: Any] {
                
                if let status = json["status"] as? [String: Int32] {
                    totalSize = status["totalsize"] ?? 0
                    freeSize = status["freesize"] ?? 0
                }
                
                if let saves = json["saves"] as? [[String: Any]] {
                    for save in saves {
                        var backupItem = BackupItem()
                        backupItem.index = save["index"] as? Int32 ?? 0
                        
                        if let filenameBase64 = save["filename"] as? String,
                           let filenameData = Data(base64Encoded: filenameBase64) {
                            backupItem.filename = String(data: filenameData, encoding: .windowsCP932) ?? filenameBase64
                        }
                        
                        if let commentBase64 = save["comment"] as? String,
                           let commentData = Data(base64Encoded: commentBase64) {
                            backupItem.comment = String(data: commentData, encoding: .windowsCP932) ?? commentBase64
                        }
                        
                        backupItem.datasize = save["datasize"] as? Int32 ?? 0
                        backupItem.blocksize = save["blocksize"] as? Int32 ?? 0
                        
                        let year = (save["year"] as? Int32 ?? 0) + 1980
                        let month = save["month"] as? Int32 ?? 1
                        let day = save["day"] as? Int32 ?? 1
                        let hour = save["hour"] as? Int32 ?? 0
                        let minute = save["minute"] as? Int32 ?? 0
                        
                        let dateFormatter = DateFormatter()
                        dateFormatter.dateFormat = "yyyy/MM/dd HH:mm:ss"
                        if let date = dateFormatter.date(from: String(format: "%04d/%02d/%02d %02d:%02d:00", year, month, day, hour, minute)) {
                            backupItem.savedate = dateFormatter.string(from: date)
                        }
                        
                        backupItems.append(backupItem)
                    }
                }
            } else {
                totalSize = 0
                freeSize = 0
            }
            
            tableView.reloadData()
            updateSumLabel()
        } catch {
            print("Failed to parse JSON: \(error.localizedDescription)")
        }
    }
    
    // MARK: - UITableViewDataSource
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return backupItems.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "BackupItemCell", for: indexPath) as! BackupItemCell
        let backupItem = backupItems[indexPath.row]
        cell.configure(with: backupItem)
        return cell
    }
    
    // MARK: - UITableViewDelegate
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        let backupItem = backupItems[indexPath.row]
        self.showBackupOption!(backupItem)
    }
    
    // MARK: - Helper Methods
    
}

class BackupItemCell: UITableViewCell {
    private let filenameLabel = UILabel()
    private let commentLabel = UILabel()
    private let savedateLabel = UILabel()
    private let datasizeLabel = UILabel()
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupViews()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupViews() {
        filenameLabel.font = UIFont.systemFont(ofSize: 16, weight: .medium)
        commentLabel.font = UIFont.systemFont(ofSize: 14)
        commentLabel.textColor = .label
        commentLabel.numberOfLines = 0  // Allow multiple lines
        savedateLabel.font = UIFont.systemFont(ofSize: 12)
        savedateLabel.textColor = .secondaryLabel
        datasizeLabel.font = UIFont.systemFont(ofSize: 12)
        datasizeLabel.textColor = .secondaryLabel
        
        [filenameLabel, commentLabel, savedateLabel, datasizeLabel].forEach {
            $0.translatesAutoresizingMaskIntoConstraints = false
            contentView.addSubview($0)
        }
        
        let dateAndSizeStack = UIStackView(arrangedSubviews: [savedateLabel, datasizeLabel])
        dateAndSizeStack.axis = .vertical
        dateAndSizeStack.alignment = .trailing
        dateAndSizeStack.spacing = 2
        dateAndSizeStack.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(dateAndSizeStack)
        
        NSLayoutConstraint.activate([
            filenameLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            filenameLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            filenameLabel.trailingAnchor.constraint(equalTo: dateAndSizeStack.leadingAnchor, constant: -8),
            
            commentLabel.topAnchor.constraint(equalTo: filenameLabel.bottomAnchor, constant: 4),
            commentLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            commentLabel.trailingAnchor.constraint(equalTo: dateAndSizeStack.leadingAnchor, constant: -8),
            commentLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8),
            
            dateAndSizeStack.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            dateAndSizeStack.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            dateAndSizeStack.widthAnchor.constraint(lessThanOrEqualTo: contentView.widthAnchor, multiplier: 0.4),  // Limit width to 40% of cell
            
            // Ensure the dateAndSizeStack doesn't push the content too far left
            dateAndSizeStack.leadingAnchor.constraint(greaterThanOrEqualTo: contentView.leadingAnchor, constant: 100)
        ])
    }
    
    func configure(with backupItem: BackupItem) {
        filenameLabel.text = backupItem.filename
        commentLabel.text = backupItem.comment
        savedateLabel.text = String( format: NSLocalizedString("Date: %@", comment: "last update date time"), backupItem.savedate ?? "")
        datasizeLabel.text = String( format: NSLocalizedString("Size: %@", comment: "File isze"), formatFileSize(Int(backupItem.datasize)))
    }
    
    private func formatFileSize(_ size: Int) -> String {
        let formatter = ByteCountFormatter()
        formatter.allowedUnits = [.useAll]
        formatter.countStyle = .file
        return formatter.string(fromByteCount: Int64(size))
    }
}


// Extension to support Windows CP932 encoding
extension String.Encoding {
    static let windowsCP932 = String.Encoding(rawValue: CFStringConvertEncodingToNSStringEncoding(CFStringEncoding(CFStringEncodings.dosJapanese.rawValue)))
}
