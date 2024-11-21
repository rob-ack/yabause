

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <windows.h>
#include "..\vulkan\Window.h"
#include "QYabVulkanWidget.h"
#include "vulkan/VIDVulkan.h"
#include "vulkan/VIDVulkanCInterface.h"
#include "VolatileSettings.h"
#include "QtYabause.h"
#include <YabauseThread.h>
#include <QResizeEvent>

QYabVulkanWidget * QYabVulkanWidget::_instance = nullptr;

QYabVulkanWidget::QYabVulkanWidget(QWidget *parent) : QWidget(parent) {
  _instance = this;
  pYabauseThread = nullptr;
  _vulkanRenderer = nullptr;

  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_NoSystemBackground);

}

QYabVulkanWidget::~QYabVulkanWidget() {
  //vkQueueWaitIdle(_vulkanRenderer->GetVulkanQueue());
  //vkDeviceWaitIdle(_vulkanRenderer->GetVulkanDevice());
  //delete _vulkanRenderer;
}

void QYabVulkanWidget::ready() {
  if (_vulkanRenderer != nullptr) {
    VIDVulkan::getInstance()->setRenderer(_vulkanRenderer);
    return;
  }
  _vulkanRenderer = new Renderer();
  VolatileSettings* vs = QtYabause::volatileSettings();
  int width = vs->value("Video/WinWidth", 800).toInt();
  int height = vs->value("Video/WinHeight", 600).toInt();
  auto w = _vulkanRenderer->OpenWindow(width, height, "[Yaba Sanshiro Vulkan] F4: Toggle full screen mode ", nullptr);
  VIDVulkan::getInstance()->setRenderer(_vulkanRenderer);
}

void QYabVulkanWidget::paintEvent(QPaintEvent* event) {
  if (pYabauseThread) pYabauseThread->execEmulation();
  update();
}

void QYabVulkanWidget::resizeEvent(QResizeEvent* event) {

  if (pYabauseThread) pYabauseThread->resize(event->size().width(), event->size().height());
  QWidget::resizeEvent(event);
}

void QYabVulkanWidget::updateView(const QSize& s) {

    const QSize size = s.isValid() ? s : this->size();
    int viewport_width_ = size.width();
    int viewport_height_ = size.height();
    if (pYabauseThread) pYabauseThread->resize(viewport_width_, viewport_height_);
}


void InitPlatform()
{

}

void DeInitPlatform()
{

}

void AddRequiredPlatformInstanceExtensions(std::vector<const char*>* instance_extensions) {
  instance_extensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
  instance_extensions->push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  instance_extensions->push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

  //instance_extensions->push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
}

void AddRequiredPlatformDeviceExtensions(std::vector<const char*>* device_extensions) {
  device_extensions->push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
  
}

void Window::_InitOSWindow() {
  
}

void Window::_DeInitOSWindow() {

}

void Window::_UpdateOSWindow() {

}

void Window::_InitOSSurface() {

  Renderer * vulkanRenderer = QYabVulkanWidget::getInstance()->getRenderer();

  HWND hwnd = reinterpret_cast<HWND>(QYabVulkanWidget::getInstance()->winId());
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = hwnd;

  VkSurfaceKHR surface;
  VkResult result = vkCreateWin32SurfaceKHR(vulkanRenderer->GetVulkanInstance(), &surfaceCreateInfo, nullptr, &surface);
  if (result != VK_SUCCESS) {
    qFatal("Failed to create Vulkan surface: %d", result);
  }

  this->_surface = surface;
  if (_surface == VK_NULL_HANDLE) {
    qFatal("Failed to retrieve Vulkan surface");
  }
}