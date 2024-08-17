import UIKit

protocol DraggableCircleViewDelegate: AnyObject {
    func onChangeTouchPos(x: UInt8, y:UInt8 )
}

func convertDoubleToUInt8(_ value: Double) -> UInt8 {
    if value < Double(UInt8.min) {
        return UInt8.min
    } else if value > Double(UInt8.max) {
        return UInt8.max
    } else {
        return UInt8(value)
    }
}

class DraggableCircleView: UIView {
    private var circleCenter: CGPoint = .zero
    private var initialCircleCenter: CGPoint = .zero
    private var currentPos: CGPoint = .zero
    private var isDragging = false
    private var inMaxPoint = false
    private let maxDistance: CGFloat = 55 // 中心点からの最大距離
    private let feedbackGenerator = UIImpactFeedbackGenerator(style: .medium)
    
    weak var delegate: DraggableCircleViewDelegate?
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setup()
    }
    

    private func setup() {
        backgroundColor = .clear
        circleCenter = CGPoint(x: bounds.midX, y: bounds.midY)
        initialCircleCenter = circleCenter
        let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
        addGestureRecognizer(panGesture)
    }
    
    override func layoutSubviews() {
         super.layoutSubviews()
         // ビューのサイズや位置が変わったときにcircleCenterを更新
         circleCenter = CGPoint(x: bounds.midX, y: bounds.midY)
         initialCircleCenter = circleCenter
         setNeedsDisplay()
     }

    override func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }
        
        // 半透明のグレー色を設定 (アルファ値を0.5に設定)
        let semiTransparentGray = UIColor.gray.withAlphaComponent(0.5).cgColor
        
        context.setFillColor(semiTransparentGray)
        context.addEllipse(in: CGRect(x: circleCenter.x - 50, y: circleCenter.y - 50, width: 100, height: 100))
        context.fillPath()
    }

    @objc private func handlePan(_ gesture: UIPanGestureRecognizer) {
        let translation = gesture.translation(in: self)
        if gesture.state == .began {
            let touchLocation = gesture.location(in: self)
            let distance = hypot(touchLocation.x - circleCenter.x, touchLocation.y - circleCenter.y)
            if distance <= 50 {
                isDragging = true
                let x:UInt8 = 128
                let y:UInt8 = 128
//                print("c: \(circleCenter.x-initialCircleCenter.x), \(circleCenter.y-initialCircleCenter.y) s: \(x), \(y)")
                delegate?.onChangeTouchPos(x: x, y: y)
            }
            
        } else if gesture.state == .changed && isDragging {
            var newCenter = CGPoint(x: circleCenter.x + translation.x, y: circleCenter.y + translation.y)
            let distanceFromCenter = hypot(newCenter.x - bounds.midX, newCenter.y - bounds.midY)
            if distanceFromCenter > maxDistance {
                let angle = atan2(newCenter.y - bounds.midY, newCenter.x - bounds.midX)
                newCenter = CGPoint(x: bounds.midX + maxDistance * cos(angle), y: bounds.midY + maxDistance * sin(angle))
                
                
                if( inMaxPoint == false ){
                    switch UIDevice.current.feedbackSupportLevel
                    {
                    case .feedbackGenerator:
                        self.feedbackGenerator.impactOccurred()
                    case .basic, .unsupported:
                        UIDevice.current.vibrate()
                    }
                    inMaxPoint = true
                }
                
            }else{
                inMaxPoint = false
            }
            circleCenter = newCenter
                
            // 相対位置を計算
            let relativeX = circleCenter.x - initialCircleCenter.x
            let relativeY = circleCenter.y - initialCircleCenter.y
            
            // 半径128の同心円にスケーリング
            let scaledX = (relativeX / maxDistance) * 128.0
            let scaledY = (relativeY / maxDistance) * 128.0
            
            // UInt8に変換
            let x = convertDoubleToUInt8(Double(scaledX) + 128.0)
            let y = convertDoubleToUInt8(Double(scaledY) + 128.0)
            
            //print("c: \(relativeX), \(relativeY) s: \(x), \(y)")
            delegate?.onChangeTouchPos(x: x, y: y)
            
            
            gesture.setTranslation(.zero, in: self)
            setNeedsDisplay()
        } else if gesture.state == .ended || gesture.state == .cancelled {
            isDragging = false
            // アニメーションで中心点に戻す（イージングをcurveEaseOutに設定）
            UIView.animate(withDuration: 0.1, delay: 0, options: .curveEaseOut, animations: {
                self.circleCenter = CGPoint(x: self.bounds.midX, y: self.bounds.midY)
                self.setNeedsDisplay()
            }, completion: nil)
            
            let x:UInt8 = 128
            let y:UInt8 = 128
            //print("c: \(circleCenter.x-initialCircleCenter.x), \(circleCenter.y-initialCircleCenter.y) s: \(x), \(y)")
            delegate?.onChangeTouchPos(x: x, y: y)

            
        }
    }
}
