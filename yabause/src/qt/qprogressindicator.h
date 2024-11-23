#ifndef QPROGRESSINDICATOR_H
#define QPROGRESSINDICATOR_H

#include <QWidget>
#include <QTimer>

class QProgressIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int delay READ animationDelay WRITE setAnimationDelay)
    Q_PROPERTY(bool displayedWhenStopped READ isDisplayedWhenStopped WRITE setDisplayedWhenStopped)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    QProgressIndicator(QWidget* parent = nullptr);

    bool isAnimated() const;
    bool isDisplayedWhenStopped() const;
    const QColor& color() const { return m_color; }
    int animationDelay() const { return m_delay; }
    virtual QSize sizeHint() const override;
    virtual int heightForWidth(int w) const override;  // 追加: heightForWidth関数の宣言

public slots:
    void startAnimation();
    void stopAnimation();
    void setAnimationDelay(int delay);
    void setDisplayedWhenStopped(bool state);
    void setColor(const QColor& color);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void timerEvent(QTimerEvent* event) override;

private:
    int m_angle;
    int m_timerId;
    int m_delay;
    bool m_displayedWhenStopped;
    QColor m_color;
};

#endif // QPROGRESSINDICATOR_H