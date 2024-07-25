import UIKit
//import MetalANGLE
import GameController
import AVFoundation
import AudioToolbox

var scurrentGamePath: UnsafeMutablePointer<Int8>? = nil

protocol GameViewControllerDelegate : AnyObject {
    func didTapMenuButton()
}

@objcMembers
class GameViewController: MGLKViewController,MGLKViewControllerDelegate {
    
    weak var gdelegate: GameViewControllerDelegate?
    
    // MARK: - Properties
    var iPodIsPlaying: UInt32 = 0
    var selectedFile: String?
    
    var context: MGLContext!
    var shareContext: MGLContext!
    var controller: GCController?
    var command: Int32 = 0
    var returnValue: Bool = false
    var keyMapper: KeyMapper!
    var isKeyRemappingMode: Bool = false
    var remapAlertController: UIAlertController?
    var currentlyMappingKey: SaturnKey = .none
    var remapLabelViews: [UILabel] = []
    var isFirst: Bool = true
    var padButtons: [PadButton] = []
    private var controller_edit_mode: Int = 0
    private var canRotateToAllOrientations: Bool = false
    private var _landscape: Bool = false
    
    // MARK: - IBOutlets
    @IBOutlet weak var leftPanel: UIImageView!
    @IBOutlet weak var rightPanel: UIImageView!
    @IBOutlet weak var startView: UIView!
    @IBOutlet weak var rightView: UIView!
    @IBOutlet weak var leftView: UIView!
    @IBOutlet weak var scaleSlider: UISlider!
    
    @IBOutlet weak var rightButton: UIView!
    @IBOutlet weak var downButton: UIView!
    @IBOutlet weak var upButton: UIView!
    @IBOutlet weak var leftButton: UIView!
    @IBOutlet weak var leftTrigger: UIView!
    @IBOutlet weak var startButton: UIView!
    @IBOutlet weak var rightTrigger: UIView!
    @IBOutlet weak var aButton: UIView!
    @IBOutlet weak var bButton: UIView!
    @IBOutlet weak var cButton: UIView!
    @IBOutlet weak var xButton: UIView!
    @IBOutlet weak var yButton: UIView!
    @IBOutlet weak var zButton: UIView!
    @IBOutlet weak var menuButton: UIButton!
    
    static let saturnKeyDescriptions: [SaturnKey: String] = [
        .a: "A",
        .b: "B",
        .c: "C",
        .x: "X",
        .y: "Y",
        .z: "Z",
        .leftTrigger: "LT",
        .rightTrigger: "RT",
        .start: "Start"
    ]

    lazy var saturnKeyToViewMappings: [SaturnKey: UIView] = {
        [
            .a: self.aButton,
            .b: self.bButton,
            .c: self.cButton,
            .x: self.xButton,
            .y: self.yButton,
            .z: self.zButton,
            .leftTrigger: self.leftTrigger,
            .rightTrigger: self.rightTrigger,
            .start: self.startButton
        ]
    }()
    
    
    // MARK: - Lifecycle Methods
    override func viewDidLoad() {
        super.viewDidLoad()
        self.delegate = self
        loadSettings()

        //sharedData = self
        _objectForLock = NSObject()
        isFirst = true
         
        glView?.drawableDepthFormat = MGLDrawableDepthFormat24
        glView?.drawableStencilFormat = MGLDrawableStencilFormat8
       
        if( scurrentGamePath != nil ){
            scurrentGamePath?.deallocate()
            scurrentGamePath = nil
        }
        let cString = (selectedFile as! NSString).utf8String!
        let bufferSize = Int(strlen(cString)) + 1
        scurrentGamePath = UnsafeMutablePointer<Int8>.allocate(capacity: bufferSize)
        strncpy(scurrentGamePath, cString, bufferSize)
        currentGamePath = UnsafePointer<CChar>(scurrentGamePath)
        
        view.isMultipleTouchEnabled = true
        command = 0
         
        rightView.backgroundColor = UIColor(red: 0, green: 0, blue: 0, alpha: 0)
        leftView.backgroundColor = UIColor(red: 0, green: 0, blue: 0, alpha: 0)
        startView.backgroundColor = UIColor(red: 0, green: 0, blue: 0, alpha: 0)
        leftButton.alpha = 0
         rightButton.alpha = 0
         upButton.alpha = 0
         downButton.alpha = 0
         aButton.alpha = 0
         bButton.alpha = 0
         cButton.alpha = 0
         xButton.alpha = 0
         yButton.alpha = 0
         zButton.alpha = 0
         leftTrigger.alpha = 0
         rightTrigger.alpha = 0
         startButton.alpha = 0
      
        // 空の配列を作成
        padButtons = [PadButton]()

        // PadButtons.lastまでの各ボタンに対して新しいPadButtonインスタンスを作成
        for _ in 0...PadButtons.last.rawValue {
            padButtons.append(PadButton())
        }
        
        // Create and add PadButton objects to the padButtons array
        padButtons[PadButtons.up.rawValue].target = upButton
        padButtons[PadButtons.right.rawValue].target = rightButton
        padButtons[PadButtons.down.rawValue].target = downButton
        padButtons[PadButtons.left.rawValue].target = leftButton
        padButtons[PadButtons.rightTrigger.rawValue].target = rightTrigger
        padButtons[PadButtons.leftTrigger.rawValue].target = leftTrigger
        padButtons[PadButtons.start.rawValue].target = startButton
        padButtons[PadButtons.a.rawValue].target = aButton
        padButtons[PadButtons.b.rawValue].target = bButton
        padButtons[PadButtons.c.rawValue].target = cButton
        padButtons[PadButtons.x.rawValue].target = xButton
        padButtons[PadButtons.y.rawValue].target = yButton
        padButtons[PadButtons.z.rawValue].target = zButton
        
         preferredFramesPerSecond = 60
         
         NotificationCenter.default.addObserver(self, selector: #selector(didEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
         NotificationCenter.default.addObserver(self, selector: #selector(didBecomeActive), name: UIApplication.didBecomeActiveNotification, object: nil)
         
         context = MGLContext(api: kMGLRenderingAPIOpenGLES3)
         shareContext = MGLContext(api: kMGLRenderingAPIOpenGLES3, sharegroup: context.sharegroup)

         guard let context = context else {
             print("Failed to create ES context")
             return
         }

         g_share_context = shareContext
         g_context = context
         
        glView!.context = context
         MGLContext.setCurrent(context)
        glLayer = glView?.glLayer
         
         setup()
         
         iOSCoreAudioInit()
         
         // ... (saturnKeyDescriptions and saturnKeyToViewMappings initialization)
         
         keyMapper = KeyMapper()
         keyMapper.loadFromDefaults()
         remapLabelViews = []

         scaleSlider.isHidden = true
         scaleSlider.minimumValue = 0.5
         scaleSlider.maximumValue = 1.0
         scaleSlider.value = _controller_scale
         scaleSlider.addTarget(self, action: #selector(didValueChanged(_:)), for: .valueChanged)
         controller_edit_mode = 0
         updateControllScale(_controller_scale)

        NotificationCenter.default.addObserver(self, selector: #selector(didEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(didBecomeActive), name: UIApplication.didBecomeActiveNotification, object: nil)
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        if hasControllerConnected() {
            print("Discovery finished on first pass")
            foundController()
        } else {
            print("Discovery happening patiently")
            patientlyDiscoverController()
        }
        
        becomeFirstResponder()
    }
    
    // MARK: - Setup Methods
    func setup() {
        MGLContext.setCurrent(context)
        let scale = UIScreen.main.scale
        let newFrame = view.frame
        let safeArea = view.safeAreaInsets
        
        if isFirst {
            start_emulation(Int32(Float(safeArea.left * scale)), Int32(Float(safeArea.bottom * scale)), Int32(Float((newFrame.width - safeArea.right - safeArea.left) * scale)), Int32(Float((newFrame.height - safeArea.top - safeArea.bottom) * scale)))
            isFirst = false
        } else {
            resize_screen(Int32(Float(safeArea.left * scale)), Int32(Float(safeArea.bottom * scale)), Int32(Float((newFrame.width - safeArea.right - safeArea.left) * scale)), Int32(Float((newFrame.height - safeArea.top - safeArea.bottom) * scale)))
        }
        returnValue = true
    }
    
    // MARK: - State Management
    func saveState() {
        command = MSG_SAVE_STATE
    }
    
    func loadState() {
        command = MSG_LOAD_STATE
    }

    func reset() {
        command = MSG_RESET
    }

    // MARK: - Controller Methods
    func hasControllerConnected() -> Bool {
        return GCController.controllers().count > 0
    }
    
    func patientlyDiscoverController() {
        GCController.startWirelessControllerDiscovery(completionHandler: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(foundController), name: .GCControllerDidConnect, object: nil)
    }
    
    @objc func foundController() {
        print("Found Controller")
        NotificationCenter.default.addObserver(self, selector: #selector(controllerDidDisconnect), name: .GCControllerDidDisconnect, object: nil)
        refreshViewsWithKeyRemaps()
        setControllerOverlayHidden(true)
        completionWirelessControllerDiscovery()
        updateSideMenu()
    }
   
    
    func mglkViewControllerUpdate(_ controller: MGLKViewController) {
        emulateOneFrame()
    }
    
    // MGLKViewDelegate メソッド
      //override func mglkView(_ view: MGLKView, drawIn rect: CGRect) {
          // 描画コード
     // }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        let scale = UIScreen.main.scale
        let newFrame = self.view.frame
        let safeArea = self.view.safeAreaInsets
        
        objc_sync_enter(_objectForLock)
        defer {
            objc_sync_exit(_objectForLock)
        }
        
        resize_screen(
            Int32(Float(safeArea.left * scale)),
            Int32(Float(safeArea.bottom * scale)),
            Int32(Float((newFrame.width - safeArea.right - safeArea.left) * scale)),
            Int32(Float((newFrame.height - safeArea.top - safeArea.bottom) * scale))
        )
    }
    
    // MARK: - Emulation Methods
    func emulateOneFrame() {
        if returnValue {
            returnValue = false
        }
        emulation_step(Int32(command))
        command = 0
    }
    
    func updateControllScale(_ scale: Float) {
        let lfv = leftView
        var tf = CGAffineTransform(scaleX: 1.0, y: 1.0)
        tf = tf.translatedBy(x: -(258.0/2.0), y: (340.0/2.0))
        tf = tf.scaledBy(x: CGFloat(scale), y: CGFloat(scale))
        tf = tf.translatedBy(x: (258.0/2.0), y: -(340.0/2.0))
        lfv!.transform = tf

        let rfv = rightView
        tf = CGAffineTransform(scaleX: 1.0, y: 1.0)
        tf = tf.translatedBy(x: (258.0/2.0), y: (340.0/2.0))
        tf = tf.scaledBy(x: CGFloat(scale), y: CGFloat(scale))
        tf = tf.translatedBy(x: -(258.0/2.0), y: -(340.0/2.0))
        rfv!.transform = tf

        let sfv = startView
        tf = CGAffineTransform(scaleX: 1.0, y: 1.0)
        //tf = tf.translatedBy(x: (258.0/2.0), y: (340.0/2.0))
        tf = tf.scaledBy(x: CGFloat(scale), y: CGFloat(scale))
        //tf = tf.translatedBy(x: -(258.0/2.0), y: -(340.0/2.0))
        sfv!.transform = tf

        let ud = UserDefaults.standard
        ud.set(scale, forKey: "controller scale")
        ud.synchronize()
    }
    
    func didValueChanged(_ slider: UISlider) {
        let scaleVal = slider.value
        updateControllScale(scaleVal)
    }
    
    var isControllerEditModeEnabled = false

    func toggleControllerEditMode() {
        isControllerEditModeEnabled.toggle()
        scaleSlider.isHidden = !isControllerEditModeEnabled
    }
    
    
    // MARK: - Pause Management
    //func setPaused(_ pause: Bool) {
        // Implement the logic for pausing the game
    //}
    
    // MARK: - Key Remapping Methods
    func startKeyRemapping() {
        refreshViewsWithKeyRemaps()
        isKeyRemappingMode.toggle()
        setControllerOverlayHidden(!isKeyRemappingMode)
    }
    
    func isCurrentlyRemappingControls() -> Bool {
        return isKeyRemappingMode
    }
    
    // MARK: - Helper Methods
    func refreshViewsWithKeyRemaps() {
        if !remapLabelViews.isEmpty {
            remapLabelViews.forEach { $0.removeFromSuperview() }
        }
        remapLabelViews.removeAll()
        
        for (button, saturnButtonView) in saturnKeyToViewMappings {
            let buttonNumber = button.rawValue
            var buttonNames = ""
            let mfiButtons = keyMapper.getControls(forMappedKey: SaturnKey(rawValue: Int(buttonNumber)) ?? SaturnKey.none )
            
            for (index, mfiButton) in mfiButtons.enumerated() {
                if index > 0 {
                    buttonNames += " / "
                }
                buttonNames += KeyMapper.controlToDisplayName(KeyMapMappableButton(rawValue: mfiButton.intValue) ?? KeyMapMappableButton.MFI_BUTTON_OPTION)
            }
            
            if !mfiButtons.isEmpty {
                let mappedLabel = UILabel()
                mappedLabel.text = buttonNames
                mappedLabel.alpha = 0.6
                mappedLabel.font = UIFont.boldSystemFont(ofSize: 16.0)
                mappedLabel.textColor = .red
                mappedLabel.translatesAutoresizingMaskIntoConstraints = false
                
                view.addSubview(mappedLabel)
                NSLayoutConstraint.activate([
                    mappedLabel.centerXAnchor.constraint(equalTo: (saturnButtonView ).centerXAnchor),
                    mappedLabel.centerYAnchor.constraint(equalTo: (saturnButtonView ).centerYAnchor)
                ])
                
                view.bringSubviewToFront(mappedLabel)
                remapLabelViews.append(mappedLabel)
                view.setNeedsLayout()
            }
        }
    }
    
    func setControllerOverlayHidden(_ hidden: Bool) {
        leftPanel.isHidden = hidden
        rightPanel.isHidden = hidden
        leftButton.isHidden = hidden
        rightButton.isHidden = hidden
        upButton.isHidden = hidden
        downButton.isHidden = hidden
        aButton.isHidden = hidden
        bButton.isHidden = hidden
        cButton.isHidden = hidden
        xButton.isHidden = hidden
        yButton.isHidden = hidden
        zButton.isHidden = hidden
        leftTrigger.isHidden = hidden
        rightTrigger.isHidden = hidden
        startButton.isHidden = hidden
        startView.isHidden = hidden
        
        remapLabelViews.forEach { label in
            label.isHidden = hasControllerConnected() && !hidden ? false : hidden
        }
    }
    
    func completionWirelessControllerDiscovery() {
        let mfiButtonHandler: (KeyMapMappableButton, Bool) -> Void = { [weak self] mfiButton, pressed in
            guard let self = self else { return }
            
            if self.isKeyRemappingMode && self.currentlyMappingKey != SaturnKey.none  {
                self.keyMapper.mapKey(self.currentlyMappingKey, toControl: mfiButton)
                if let remapAlertController = self.remapAlertController {
                    remapAlertController.dismiss(animated: true, completion: nil)
                }
                self.keyMapper.saveKeyMapping()
                self.currentlyMappingKey = SaturnKey.none
                self.refreshViewsWithKeyRemaps()
            } else {
                if let mappedKeyNumber = self.keyMapper.getMappedKey(forControl: mfiButton) {
                    let mappedKey = SaturnKey(rawValue: mappedKeyNumber.intValue) ?? .none
                    if pressed {
                        PerKeyDown(UInt32(mappedKey.rawValue))
                    } else {
                        PerKeyUp(UInt32(mappedKey.rawValue))
                    }
                }
            }
        }
        
        if let controller = GCController.controllers().first {
            self.controller = controller
            if let gamepad = controller.extendedGamepad {
                gamepad.buttonHome?.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_HOME, pressed)
                }
                
                gamepad.buttonMenu.valueChangedHandler = { [weak self] _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_MENU, pressed)
//                    if let revealViewController = self?.revealViewController as? GameRevealViewController, pressed {
//                        revealViewController.revealToggle(0)
 //                   }
                }
                
                gamepad.buttonOptions?.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_OPTION, pressed)
                }
                
                gamepad.buttonA.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_A, pressed)
                }
                
                gamepad.rightShoulder.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_RS, pressed)
                }
                
                gamepad.leftShoulder.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_LS, pressed)
                }
                
                gamepad.leftTrigger.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_LT, pressed)
                }
                
                gamepad.rightTrigger.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_RT, pressed)
                }
                
                gamepad.buttonX.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_X, pressed)
                }
                
                gamepad.buttonY.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_Y, pressed)
                }
                
                gamepad.buttonB.valueChangedHandler = { _, _, pressed in
                    mfiButtonHandler(.MFI_BUTTON_B, pressed)
                }
                
                gamepad.dpad.up.valueChangedHandler = { _, _, pressed in
                    if pressed {
                        PerKeyDown(UInt32(SaturnKey.up.rawValue))
                    } else {
                        PerKeyUp(UInt32(SaturnKey.up.rawValue))
                    }
                }
                
                gamepad.dpad.down.valueChangedHandler = { _, _, pressed in
                    if pressed {
                        PerKeyDown(UInt32(SaturnKey.down.rawValue))
                    } else {
                        PerKeyUp(UInt32(SaturnKey.down.rawValue))
                    }
                }
                
                gamepad.dpad.left.valueChangedHandler = { _, _, pressed in
                    if pressed {
                        PerKeyDown(UInt32(SaturnKey.left.rawValue))
                    } else {
                        PerKeyUp(UInt32(SaturnKey.left.rawValue))
                    }
                }
                
                gamepad.dpad.right.valueChangedHandler = { _, _, pressed in
                    if pressed {
                        PerKeyDown(UInt32(SaturnKey.right.rawValue))
                    } else {
                        PerKeyUp(UInt32(SaturnKey.right.rawValue))
                    }
                }
                
                // Commented out in the original code
                /*
                gamepad.rightThumbstick.valueChangedHandler = { _, xValue, yValue in
                    if yValue >= 0.5 || yValue <= -0.5 {
                        PerKeyDown(.start)
                    } else {
                        PerKeyUp(.start)
                    }
                }
                */
            }
        }
    }
    
    @IBAction func onMenu(_ sender: Any) {
        gdelegate?.didTapMenuButton()
    }
    
    func updateSideMenu() {
        // Implement the logic for updating side menu
    }
    
    // MARK: - Notification Handlers
    @objc func didEnterBackground() {
        enterBackGround()
        self.isPaused = true
    }
    
    @objc func didBecomeActive() {
        self.view.becomeFirstResponder()
        self.isPaused = false
        self.returnValue = true
    }
    
    override var canBecomeFirstResponder: Bool {
        return true
    }
    
    @objc func controllerDidDisconnect() {
        // Implement the logic for when a controller disconnects
    }
    
    // MARK: settings
    func loadSettings() {
        let bundle = Bundle.main
        let path = getSettingFilePath()
        if let dic = NSDictionary(contentsOfFile: path) {
            _bios = ObjCBool((dic["builtin bios"] as? Bool) ?? false)
            _cart = Int32((dic["cartridge"] as? Int) ?? 0)
            _fps = ObjCBool((dic["show fps"] as? Bool) ?? false)
            _frame_skip = ObjCBool((dic["frame skip"] as? Bool) ?? false)
            _aspect_rate = ObjCBool((dic["keep aspect rate"] as? Bool) ?? false)
            _filter = Int32(0)
            _sound_engine = Int32((dic["sound engine"] as? Int) ?? 0)
            _rendering_resolution = Int32((dic["rendering resolution"] as? Int) ?? 0)
            _rotate_screen = ObjCBool((dic["rotate screen"] as? Bool) ?? false)
        }
        
        let ud = UserDefaults.standard
        let defaults: [String: Any] = [
            "controller scale": "0.8",
            "landscape": true
        ]
        ud.register(defaults: defaults)
        
        _controller_scale = ud.float(forKey: "controller scale")
         self._landscape = ud.bool(forKey: "landscape")
    }

    func getSettingFilePath() -> String {
        let fileManager = FileManager.default
        let fileName = "settings.plist"
        
        let paths = NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)
        let libraryDirectory = paths[0]
        
        let filePath = (libraryDirectory as NSString).appendingPathComponent(fileName)
        print("full path name: \(filePath)")
        
        // check if file exists
        if fileManager.fileExists(atPath: filePath) {
            print("File exists")
        } else {
            let bundle = Bundle.main
            if let path = bundle.path(forResource: "settings", ofType: "plist") {
                do {
                    try fileManager.copyItem(atPath: path, toPath: filePath)
                } catch {
                    print("Error copying file: \(error)")
                }
            }
        }
        
        return filePath
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
//        super.touchesBegan(touches, with: event)
        for touch in touches {
            for btnindex in 0..<PadButtons.last.rawValue {
                if let target = padButtons[Int(btnindex)].target {
                    let point = touch.location(in: target)
                    if target.bounds.contains(point) {
                        padButtons[Int(btnindex)].on(touch)
//                        print("PAD:\(btnindex) on")
                    } else {
                        // print("\(btnindex): [\(Int(target.frame.origin.x)),\(Int(target.frame.origin.y)),\(Int(target.bounds.size.width)),\(Int(target.bounds.size.height))]-[\(Int(point.x)),\(Int(point.y))]")
                    }
                }
            }
        }
        
        if hasControllerConnected() { return }
        
        for btnindex in 0..<PadButtons.last.rawValue {
            if padButtons[Int(btnindex)].isOn() {
                PerKeyDown(UInt32(btnindex))
            } else {
                PerKeyUp(UInt32(btnindex))
            }
        }
    }
    
    override func pressesBegan(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
        var handleEvent = 0
        
        for press in presses {
            guard let key = press.key else { continue }
            
            switch key.charactersIgnoringModifiers {
            case UIKeyCommand.inputLeftArrow:
                PerKeyDown(UInt32(PadButtons.left.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputRightArrow:
                PerKeyDown(UInt32(PadButtons.right.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputUpArrow:
                PerKeyDown(UInt32(PadButtons.up.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputDownArrow:
                PerKeyDown(UInt32(PadButtons.down.rawValue))
                handleEvent += 1
            case "\r":
                PerKeyDown(UInt32(PadButtons.start.rawValue))
                handleEvent += 1
            case "z":
                PerKeyDown(UInt32(PadButtons.a.rawValue))
                handleEvent += 1
            case "x":
                PerKeyDown(UInt32(PadButtons.b.rawValue))
                handleEvent += 1
            case "c":
                PerKeyDown(UInt32(PadButtons.c.rawValue))
                handleEvent += 1
            case "a":
                PerKeyDown(UInt32(PadButtons.x.rawValue))
                handleEvent += 1
            case "s":
                PerKeyDown(UInt32(PadButtons.y.rawValue))
                handleEvent += 1
            case "d":
                PerKeyDown(UInt32(PadButtons.z.rawValue))
                handleEvent += 1
            case "q":
                PerKeyDown(UInt32(PadButtons.leftTrigger.rawValue))
                handleEvent += 1
            case "e":
                PerKeyDown(UInt32(PadButtons.rightTrigger.rawValue))
                handleEvent += 1
            default:
                break
            }
        }
        
        if handleEvent == 0 {
            super.pressesBegan(presses, with: event)
        }
    }
    
    override func pressesEnded(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
        var handleEvent = 0
        
        for press in presses {
            guard let key = press.key else { continue }
            
            switch key.charactersIgnoringModifiers {
            case UIKeyCommand.inputLeftArrow:
                PerKeyUp(UInt32(PadButtons.left.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputRightArrow:
                PerKeyUp(UInt32(PadButtons.right.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputUpArrow:
                PerKeyUp(UInt32(PadButtons.up.rawValue))
                handleEvent += 1
            case UIKeyCommand.inputDownArrow:
                PerKeyUp(UInt32(PadButtons.down.rawValue))
                handleEvent += 1
            case "\r":
                PerKeyUp(UInt32(PadButtons.start.rawValue))
                handleEvent += 1
            case "z":
                PerKeyUp(UInt32(PadButtons.a.rawValue))
                handleEvent += 1
            case "x":
                PerKeyUp(UInt32(PadButtons.b.rawValue))
                handleEvent += 1
            case "c":
                PerKeyUp(UInt32(PadButtons.c.rawValue))
                handleEvent += 1
            case "a":
                PerKeyUp(UInt32(PadButtons.x.rawValue))
                handleEvent += 1
            case "s":
                PerKeyUp(UInt32(PadButtons.y.rawValue))
                handleEvent += 1
            case "d":
                PerKeyUp(UInt32(PadButtons.z.rawValue))
                handleEvent += 1
            case "q":
                PerKeyUp(UInt32(PadButtons.leftTrigger.rawValue))
                handleEvent += 1
            case "e":
                PerKeyUp(UInt32(PadButtons.rightTrigger.rawValue))
                handleEvent += 1
            default:
                break
            }
        }
        
        if handleEvent == 0 {
            super.pressesEnded(presses, with: event)
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            if padButtons[Int(PadButtons.down.rawValue)].isOn() && padButtons[Int(PadButtons.down.rawValue)].getPointId() == touch {
                if let target = padButtons[Int(PadButtons.down.rawValue)].target {
                    let point = touch.location(in: target)
                    if point.y < 0 {
                        padButtons[Int(PadButtons.down.rawValue)].off()
                    }
                }
            }
            
            if padButtons[Int(PadButtons.up.rawValue)].isOn() && padButtons[Int(PadButtons.up.rawValue)].getPointId() == touch {
                if let target = padButtons[PadButtons.up.rawValue].target {
                    let point = touch.location(in: target)
                    if point.y > target.bounds.size.height {
                        padButtons[PadButtons.up.rawValue].off()
                    }
                }
            }
            
            if let target = padButtons[PadButtons.down.rawValue].target {
                let point = touch.location(in: target)
                if target.bounds.contains(point) {
                    padButtons[PadButtons.down.rawValue].on(touch)
                }
            }
            
            if let target = padButtons[PadButtons.up.rawValue].target {
                let point = touch.location(in: target)
                if target.bounds.contains(point) {
                    padButtons[PadButtons.up.rawValue].on(touch)
                }
            }
            
            if padButtons[PadButtons.right.rawValue].isOn() && padButtons[PadButtons.right.rawValue].getPointId() == touch {
                if let target = padButtons[PadButtons.right.rawValue].target {
                    let point = touch.location(in: target)
                    if point.x < 0 {
                        padButtons[PadButtons.right.rawValue].off()
                    }
                }
            }
            
            if padButtons[PadButtons.left.rawValue].isOn() && padButtons[PadButtons.left.rawValue].getPointId() == touch {
                if let target = padButtons[PadButtons.left.rawValue].target {
                    let point = touch.location(in: target)
                    if point.x > target.bounds.size.width {
                        padButtons[PadButtons.left.rawValue].off()
                    }
                }
            }
            
            if let target = padButtons[PadButtons.left.rawValue].target {
                let point = touch.location(in: target)
                if target.bounds.contains(point) {
                    padButtons[PadButtons.left.rawValue].on(touch)
                }
            }
            
            if let target = padButtons[PadButtons.right.rawValue].target {
                let point = touch.location(in: target)
                if target.bounds.contains(point) {
                    padButtons[PadButtons.right.rawValue].on(touch)
                }
            }
            
            for btnindex in PadButtons.rightTrigger.rawValue..<PadButtons.last.rawValue {
                if let target = padButtons[btnindex].target {
                    let point = touch.location(in: target)
                    
                    if padButtons[btnindex].getPointId() == touch {
                        if !target.bounds.contains(point) {
                            padButtons[btnindex].off()
                        }
                    } else {
                        if target.bounds.contains(point) {
                            padButtons[btnindex].on(touch)
                        }
                    }
                }
            }
        }
        
        if hasControllerConnected() { return }
        
        for btnindex in 0..<PadButtons.last.rawValue {
            if padButtons[btnindex].isOn() {
                PerKeyDown(UInt32(btnindex))
            } else {
                PerKeyUp(UInt32(btnindex))
            }
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            for btnindex in 0..<PadButtons.last.rawValue {
                if padButtons[btnindex].isOn() && padButtons[btnindex].getPointId() == touch {
                    padButtons[btnindex].off()
                    // print("touchesEnded PAD:\(btnindex) Up")
                    if isKeyRemappingMode {
                        showRemapControlAlert(withSaturnKey: SaturnKey(rawValue: btnindex) ?? .none)
                    }
                }
            }
        }
        
        if hasControllerConnected() { return }
        
        for btnindex in 0..<PadButtons.last.rawValue {
            if padButtons[btnindex].isOn() {
                PerKeyDown(UInt32(btnindex))
            } else {
                PerKeyUp(UInt32(btnindex))
            }
        }
    }
    
    func showRemapControlAlert(withSaturnKey saturnKey: SaturnKey) {
        let keyDescription = GameViewController.saturnKeyDescriptions[saturnKey] ?? ""
        remapAlertController = UIAlertController(
            title: "Remap Key",
            message: "Press a button to map the [\(keyDescription)] key",
            preferredStyle: .alert
        )
        
        let cancel = UIAlertAction(title: "Cancel", style: .cancel) { [weak self] _ in
            self?.isKeyRemappingMode = false
            self?.setControllerOverlayHidden(true)
            self?.remapAlertController?.dismiss(animated: true, completion: nil)
            self?.currentlyMappingKey = .none
        }
        
        let unbind = UIAlertAction(title: "Unbind", style: .default) { [weak self] _ in
            self?.keyMapper.unmapKey(saturnKey)
            self?.currentlyMappingKey = .none
            self?.refreshViewsWithKeyRemaps()
            self?.remapAlertController?.dismiss(animated: true, completion: nil)
        }
        
        isKeyRemappingMode = true
        remapAlertController?.addAction(cancel)
        remapAlertController?.addAction(unbind)
        currentlyMappingKey = saturnKey
        present(remapAlertController!, animated: true, completion: nil)
    }
    
    func toggleControllSetting(){
        if(hasControllerConnected()){
            startKeyRemapping()
        }else{
            toggleControllerEditMode()
        }
    }
    
    func presentFileSelectViewController() {
        
        self.command = MSG_OPEN_TRAY
        
        let fsVC = FileSelectController()
        fsVC.completionHandler = { data in
            if let data = data {
                self.selectedFile = data
                if( scurrentGamePath != nil ){
                    scurrentGamePath?.deallocate()
                    scurrentGamePath = nil
                }
                let cString = (self.selectedFile! as NSString).utf8String!
                let bufferSize = Int(strlen(cString)) + 1
                scurrentGamePath = UnsafeMutablePointer<Int8>.allocate(capacity: bufferSize)
                strncpy(scurrentGamePath, cString, bufferSize)
                currentGamePath = UnsafePointer<CChar>(scurrentGamePath)
                self.command = MSG_CLOSE_TRAY
            }else{
                self.command = MSG_CLOSE_TRAY
            }
        }
        
        
        present(fsVC, animated: true, completion: nil)
    }
    
}
