/*
    nanogui/imageview.cpp -- Widget used to display images.

    The image view widget was contributed by Stefan Ivanov.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/imageview.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>
#include <SDL2/SDL.h>
#include <nanogui/theme.h>
#include <nanovg.h>
#include <cmath>

NAMESPACE_BEGIN(nanogui)

ImageView::ImageView(Widget* parent)
  : Widget(parent),
  mImageID(-1),
  mScale(1.0f),
  mOffset(Vector2f::Zero()),
  mFixedScale(false),
  mFixedOffset(false),
  mPixelInfoCallback(nullptr)
{}

ImageView::~ImageView() {}

void ImageView::bindImage(uint32_t imageId)
{
  mImageID = imageId;
  mNeedUpdate = true;
}


void ImageView::updateImageParameters()
{
  int32_t w, h;
  nvgImageSize(screen()->nvgContext(), mImageID, &w, &h);
  mImageSize = Vector2i(w, h);
}

void ImageView::_internalDraw(NVGcontext* ctx)
{
  Vector2f scaleFactor(mScale, mScale);
  Vector2f positionInScreen = position().cast<float>();
  Vector2f positionAfterOffset = positionInScreen + mOffset;
  Vector2f imagePosition = positionAfterOffset;

  NVGpaint imgPaint = nvgImagePattern(ctx, imagePosition.x(), imagePosition.y(),
    mImageSize.x() * scaleFactor.x(), mImageSize.y() * scaleFactor.y(), 0, mImageID, 1.0);

  nvgBeginPath(ctx);
  nvgRoundedRect(ctx, imagePosition.x(), imagePosition.y(),
    mImageSize.x() * scaleFactor.x(), mImageSize.y() * scaleFactor.y(),
    5);
  nvgFillPaint(ctx, imgPaint);
  nvgFill(ctx);
}

Vector2f ImageView::imageCoordinateAt(const Vector2f& position) const {
  auto imagePosition = position - mOffset;
  return imagePosition / mScale;
}

Vector2f ImageView::clampedImageCoordinateAt(const Vector2f& position) const {
  auto imageCoordinate = imageCoordinateAt(position);
  return imageCoordinate.array().
    max(Vector2f::Zero().array()).
    min(imageSizeF().array());
}

Vector2f ImageView::positionForCoordinate(const Vector2f& imageCoordinate) const {
  return mScale * imageCoordinate + mOffset;
}

void ImageView::setImageCoordinateAt(const Vector2f& position, const Vector2f& imageCoordinate) {
  // Calculate where the new offset must be in order to satisfy the image position equation.
  // Round the floating point values to balance out the floating point to integer conversions.
  mOffset = position - (imageCoordinate * mScale); // .unaryExpr([](float x) { return std::round(x); });
  // Clamp offset so that the image remains near the screen.
  mOffset = mOffset.array().
    min(sizeF().array()).
    max(-scaledImageSizeF().array());
}

void ImageView::center() {
  mOffset = (sizeF() - scaledImageSizeF()) / 2;
}

void ImageView::fit() {
  // Calculate the appropriate scaling factor.
  mScale = (sizeF().cwiseQuotient(imageSizeF())).minCoeff();
  center();
}

void ImageView::setScaleCentered(float scale) {
  auto centerPosition = sizeF() / 2;
  auto p = imageCoordinateAt(centerPosition);
  mScale = scale;
  setImageCoordinateAt(centerPosition, p);
}

void ImageView::moveOffset(const Vector2f& delta) {
  // Apply the delta to the offset.
  mOffset += delta;

  // Prevent the image from going out of bounds.
  auto scaledSize = scaledImageSizeF();
  if (mOffset.x() + scaledSize.x() < 0)
    mOffset.x() = -scaledSize.x();
  if (mOffset.x() > sizeF().x())
    mOffset.x() = sizeF().x();
  if (mOffset.y() + scaledSize.y() < 0)
    mOffset.y() = -scaledSize.y();
  if (mOffset.y() > sizeF().y())
    mOffset.y() = sizeF().y();
}

void ImageView::zoom(int amount, const Vector2f& focusPosition) {
  auto focusedCoordinate = imageCoordinateAt(focusPosition);
  float scaleFactor = std::pow(mZoomSensitivity, amount);
  mScale = std::max(0.01f, scaleFactor * mScale);
  setImageCoordinateAt(focusPosition, focusedCoordinate);
}

bool ImageView::mouseDragEvent(const Vector2i& p, const Vector2i& rel, int button, int /*modifiers*/) {
  if ((button & (1 << SDL_BUTTON_LEFT)) != 0 && !mFixedOffset) {
    setImageCoordinateAt((p + rel).cast<float>(), imageCoordinateAt(p.cast<float>()));
    return true;
  }
  return false;
}

bool ImageView::gridVisible() const {
  return (mGridThreshold != -1) && (mScale > mGridThreshold);
}

bool ImageView::pixelInfoVisible() const {
  return mPixelInfoCallback && (mPixelInfoThreshold != -1) && (mScale > mPixelInfoThreshold);
}

bool ImageView::helpersVisible() const {
  return gridVisible() || pixelInfoVisible();
}

bool ImageView::scrollEvent(const Vector2i& p, const Vector2f& rel) {
  if (mFixedScale)
    return false;
  float v = rel.y();
  if (std::abs(v) < 1)
    v = std::copysign(1.f, v);
  zoom(v, (p - position()).cast<float>());
  return true;
}

bool ImageView::keyboardEvent(int key, int /*scancode*/, int action, int modifiers) {
  if (action) {
    switch (key) {
    case SDLK_LEFT:
      if (!mFixedOffset) {
        if (SDLK_LCTRL & modifiers)
          moveOffset(Vector2f(30, 0));
        else
          moveOffset(Vector2f(10, 0));
        return true;
      }
      break;
    case SDLK_RIGHT:
      if (!mFixedOffset) {
        if (SDLK_LCTRL & modifiers)
          moveOffset(Vector2f(-30, 0));
        else
          moveOffset(Vector2f(-10, 0));
        return true;
      }
      break;
    case SDLK_DOWN:
      if (!mFixedOffset) {
        if (SDLK_LCTRL & modifiers)
          moveOffset(Vector2f(0, -30));
        else
          moveOffset(Vector2f(0, -10));
        return true;
      }
      break;
    case SDLK_UP:
      if (!mFixedOffset) {
        if (SDLK_LCTRL & modifiers)
          moveOffset(Vector2f(0, 30));
        else
          moveOffset(Vector2f(0, 10));
        return true;
      }
      break;
    }
  }
  return false;
}

bool ImageView::keyboardCharacterEvent(unsigned int codepoint) {
  switch (codepoint) {
  case '-':
    if (!mFixedScale) {
      zoom(-1, sizeF() / 2);
      return true;
    }
    break;
  case '+':
    if (!mFixedScale) {
      zoom(1, sizeF() / 2);
      return true;
    }
    break;
  case 'c':
    if (!mFixedOffset) {
      center();
      return true;
    }
    break;
  case 'f':
    if (!mFixedOffset && !mFixedScale) {
      fit();
      return true;
    }
    break;
  case '1': case '2': case '3': case '4': case '5':
  case '6': case '7': case '8': case '9':
    if (!mFixedScale) {
      setScaleCentered(1 << (codepoint - '1'));
      return true;
    }
    break;
  default:
    return false;
  }
  return false;
}

Vector2i ImageView::preferredSize(NVGcontext* /*ctx*/) const {
  return mImageSize;
}

void ImageView::performLayout(NVGcontext* ctx) {
  Widget::performLayout(ctx);
  center();
}

void ImageView::draw(NVGcontext* ctx)
{
  Widget::draw(ctx);

  if (mNeedUpdate)
  {
    mNeedUpdate = false;
    updateImageParameters();
    fit();
  }
  drawImageBorder(ctx);

  // Calculate several variables that need to be send to OpenGL in order for the image to be
  // properly displayed inside the widget.

  _internalDraw(ctx);

  if (helpersVisible())
    drawHelpers(ctx);

  drawWidgetBorder(ctx);
}

void ImageView::drawWidgetBorder(NVGcontext* ctx) const {
  nvgBeginPath(ctx);
  nvgStrokeWidth(ctx, 1.0f);
  nvgRoundedRect(ctx, mPos.x() - 0.5f, mPos.y() - 0.5f,
    mSize.x() + 1, mSize.y() + 1, mTheme->mWindowCornerRadius);
  nvgStrokeColor(ctx, mTheme->mBorderLight);
  nvgRoundedRect(ctx, mPos.x() - 1.0f, mPos.y() - 1.0f,
    mSize.x() + 2, mSize.y() + 2, mTheme->mWindowCornerRadius);
  nvgStrokeColor(ctx, mTheme->mBorderDark);
  nvgStroke(ctx);
}

void ImageView::drawImageBorder(NVGcontext* ctx) const {
  nvgSave(ctx);
  nvgBeginPath(ctx);
  nvgScissor(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
  nvgStrokeWidth(ctx, 1.0f);
  Vector2i borderPosition = mPos + mOffset.cast<int>();
  Vector2i borderSize = scaledImageSizeF().cast<int>();
  nvgRect(ctx, borderPosition.x() - 0.5f, borderPosition.y() - 0.5f,
    borderSize.x() + 1, borderSize.y() + 1);
  nvgStrokeColor(ctx, Color(1.0f, 1.0f, 1.0f, 1.0f));
  nvgStroke(ctx);
  nvgResetScissor(ctx);
  nvgRestore(ctx);
}

void ImageView::drawHelpers(NVGcontext* ctx) const {
  // We need to apply mPos after the transformation to account for the position of the widget
  // relative to the parent.
  Vector2f upperLeftCorner = positionForCoordinate(Vector2f(0, 0)) + positionF();
  Vector2f lowerRightCorner = positionForCoordinate(imageSizeF()) + positionF();
  // Use the scissor method in NanoVG to display only the correct part of the grid.
  Vector2f scissorPosition = upperLeftCorner.array().max(positionF().array());
  Vector2f sizeOffsetDifference = sizeF() - mOffset;
  Vector2f scissorSize = sizeOffsetDifference.array().min(sizeF().array());
  nvgSave(ctx);
  nvgScissor(ctx, scissorPosition.x(), scissorPosition.y(), scissorSize.x(), scissorSize.y());
  if (gridVisible())
    drawPixelGrid(ctx, upperLeftCorner, lowerRightCorner, mScale);
  if (pixelInfoVisible())
    drawPixelInfo(ctx, mScale);
  nvgRestore(ctx);
}

void ImageView::drawPixelGrid(NVGcontext* ctx, const Vector2f& upperLeftCorner,
  const Vector2f& lowerRightCorner, const float stride) {
  nvgBeginPath(ctx);
  // Draw the vertical lines for the grid
  float currentX = std::floor(upperLeftCorner.x());
  while (currentX <= lowerRightCorner.x()) {
    nvgMoveTo(ctx, std::floor(currentX), std::floor(upperLeftCorner.y()));
    nvgLineTo(ctx, std::floor(currentX), std::floor(lowerRightCorner.y()));
    currentX += stride;
  }
  // Draw the horizontal lines for the grid.
  float currentY = std::floor(upperLeftCorner.y());
  while (currentY <= lowerRightCorner.y()) {
    nvgMoveTo(ctx, std::floor(upperLeftCorner.x()), std::floor(currentY));
    nvgLineTo(ctx, std::floor(lowerRightCorner.x()), std::floor(currentY));
    currentY += stride;
  }
  nvgStrokeWidth(ctx, 1.0f);
  //Color c(1.0f, 1.0f, 1.0f, 1.0f);
  nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 255));
  nvgStroke(ctx);
}

void ImageView::drawPixelInfo(NVGcontext* ctx, const float stride) const {
  // Extract the image coordinates at the two corners of the widget.
  Vector2f currentPixelF = clampedImageCoordinateAt(Vector2f::Zero());
  Vector2f lastPixelF = clampedImageCoordinateAt(sizeF());
  // Round the top left coordinates down and bottom down coordinates up.
  // This is done so that the edge information does not pop up suddenly when it gets in range.
  currentPixelF = currentPixelF.unaryExpr([](float x) { return std::floor(x); });
  lastPixelF = lastPixelF.unaryExpr([](float x) { return std::ceil(x); });
  Vector2i currentPixel = currentPixelF.cast<int>();
  Vector2i lastPixel = lastPixelF.cast<int>();

  // Extract the positions for where to draw the text.
  Vector2f currentCellPosition = (positionF() + positionForCoordinate(currentPixelF));
  float xInitialPosition = currentCellPosition.x();
  int xInitialIndex = currentPixel.x();

  // Properly scale the pixel information for the given stride.
  auto fontSize = stride * mFontScaleFactor;
  static constexpr float maxFontSize = 30.0f;
  fontSize = fontSize > maxFontSize ? maxFontSize : fontSize;
  nvgSave(ctx);
  nvgBeginPath(ctx);
  nvgFontSize(ctx, fontSize);
  nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
  nvgFontFace(ctx, "sans");
  while (currentPixel.y() != lastPixel.y()) {
    while (currentPixel.x() != lastPixel.x()) {
      writePixelInfo(ctx, currentCellPosition, currentPixel, stride);
      currentCellPosition.x() += stride;
      ++currentPixel.x();
    }
    currentCellPosition.x() = xInitialPosition;
    currentCellPosition.y() += stride;
    ++currentPixel.y();
    currentPixel.x() = xInitialIndex;
  }
  nvgRestore(ctx);
}

namespace {
  std::vector<std::string> splitString(const std::string& text, const std::string& delimiter) {
    using std::string; using std::vector;
    vector<string> strings;
    string::size_type current = 0;
    string::size_type previous = 0;
    while ((current = text.find(delimiter, previous)) != string::npos) {
      strings.push_back(text.substr(previous, current - previous));
      previous = current + 1;
    }
    strings.push_back(text.substr(previous));
    return strings;
  }
}

void ImageView::writePixelInfo(NVGcontext* ctx, const Vector2f& cellPosition,
  const Vector2i& pixel, const float stride) const {
  auto pixelData = mPixelInfoCallback(pixel);
  auto pixelDataRows = splitString(pixelData.first, "\n");

  // If no data is provided for this pixel then simply return.
  if (pixelDataRows.empty())
    return;

  nvgFillColor(ctx, pixelData.second);
  auto padding = stride / 10;
  auto maxSize = stride - 2 * padding;

  // Measure the size of a single line of text.
  float bounds[4];
  nvgTextBoxBounds(ctx, 0.0f, 0.0f, maxSize, pixelDataRows.front().data(), nullptr, bounds);
  auto rowHeight = bounds[3] - bounds[1];
  auto totalRowsHeight = rowHeight * pixelDataRows.size();

  // Choose the initial y offset and the index for the past the last visible row.
  auto yOffset = 0.0f;
  auto lastIndex = 0;

  if (totalRowsHeight > maxSize) {
    yOffset = padding;
    lastIndex = (int)(maxSize / rowHeight);
  }
  else {
    yOffset = (stride - totalRowsHeight) / 2;
    lastIndex = (int)pixelDataRows.size();
  }

  for (int i = 0; i != lastIndex; ++i) {
    nvgText(ctx, cellPosition.x() + stride / 2, cellPosition.y() + yOffset,
      pixelDataRows[i].data(), nullptr);
    yOffset += rowHeight;
  }
}


NAMESPACE_END(nanogui)

#if 0

NAMESPACE_BEGIN(nanogui)

namespace {
    std::vector<std::string> splitString(const std::string& text, const std::string& delimiter) {
        using std::string; using std::vector;
        vector<string> strings;
        string::size_type current = 0;
        string::size_type previous = 0;
        while ((current = text.find(delimiter, previous)) != string::npos) {
            strings.push_back(text.substr(previous, current - previous));
            previous = current + 1;
        }
        strings.push_back(text.substr(previous));
        return strings;
    }
#if defined(NANOVG_GL3_IMPLEMENTATION)
    constexpr char const *const defaultImageViewVertexShader =
        R"(#version 330
        uniform vec2 scaleFactor;
        uniform vec2 position;
        in vec2 vertex;
        out vec2 uv;
        void main() {
            uv = vertex;
            vec2 scaledVertex = (vertex * scaleFactor) + position;
            gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                                1.0 - 2.0*scaledVertex.y,
                                0.0, 1.0);

        })";

    constexpr char const *const defaultImageViewFragmentShader =
        R"(#version 330
        uniform sampler2D image;
        out vec4 color;
        in vec2 uv;
        void main() {
            color = texture(image, uv);
        })";
#elif defined(NANOVG_GL2_IMPLEMENTATION)
    constexpr char const *const defaultImageViewVertexShader =
        R"(#version 100
        uniform vec2 scaleFactor;
        uniform vec2 position;
        attribute vec2 vertex;
        varying vec2 uv;
        void main() {
            uv = vertex;
            vec2 scaledVertex = (vertex * scaleFactor) + position;
            gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                                1.0 - 2.0*scaledVertex.y,
                                0.0, 1.0);

        })";

    constexpr char const *const defaultImageViewFragmentShader =
        R"(#version 100
        #ifdef GL_FRAGMENT_PRECISION_HIGH
        # define maxfragp highp
        #else
        # define maxfragp medp
        #endif
        uniform sampler2D image;
        varying maxfragp vec2 uv;
        void main() {
            gl_FragColor = texture2D(image, uv);
        })";
#elif defined(NANOVG_GLES3_IMPLEMENTATION)
    constexpr char const *const defaultImageViewVertexShader =
        R"(#version 300 es
        uniform vec2 scaleFactor;
        uniform vec2 position;
        in vec2 vertex;
        out vec2 uv;
        void main() {
            uv = vertex;
            vec2 scaledVertex = (vertex * scaleFactor) + position;
            gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                                1.0 - 2.0*scaledVertex.y,
                                0.0, 1.0);

        })";

    constexpr char const *const defaultImageViewFragmentShader =
        R"(#version 300 es
        #ifdef GL_FRAGMENT_PRECISION_HIGH
        # define maxfragp highp
        #else
        # define maxfragp medp
        #endif
        uniform sampler2D image;
        in maxfragp vec2 uv;
        out maxfragp vec4 color;
        void main() {
            color = texture(image, uv);
        })";
#elif defined(NANOVG_GLES2_IMPLEMENTATION)
    constexpr char const *const defaultImageViewVertexShader =
        R"(#version 100
        uniform vec2 scaleFactor;
        uniform vec2 position;
        attribute vec2 vertex;
        varying vec2 uv;
        void main() {
            uv = vertex;
            vec2 scaledVertex = (vertex * scaleFactor) + position;
            gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                                1.0 - 2.0*scaledVertex.y,
                                0.0, 1.0);

        })";

    constexpr char const *const defaultImageViewFragmentShader =
        R"(#version 100
        #ifdef GL_FRAGMENT_PRECISION_HIGH
        # define maxfragp highp
        #else
        # define maxfragp medp
        #endif
        uniform sampler2D image;
        varying maxfragp vec2 uv;
        void main() {
            gl_FragColor = texture2D(image, uv);
        })";
#elif defined(NANOVG_VULKAN_IMPLEMENTATION)
    constexpr char const *const defaultImageViewVertexShader = "";
    constexpr char const *const defaultImageViewFragmentShader = "";
#endif

}

ImageView::ImageView(Widget* parent, const GLTexture &texture)
    : Widget(parent), mTexture(texture), mScale(1.0f), mOffset(Vector2f::Zero()),
    mFixedScale(false), mFixedOffset(false), mPixelInfoCallback(nullptr) {

#if !defined(NANOVG_VULKAN_IMPLEMENTATION)
    updateImageParameters();
    mShader.init("ImageViewShader", defaultImageViewVertexShader,
                 defaultImageViewFragmentShader);

    MatrixXu indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 1;

    MatrixXf vertices(2, 4);
    vertices.col(0) << 0, 0;
    vertices.col(1) << 1, 0;
    vertices.col(2) << 0, 1;
    vertices.col(3) << 1, 1;

    mShader.bind();
    mShader.uploadIndices(indices);
    mShader.uploadAttrib("vertex", vertices);
    mShader.unbind();
#endif
}

ImageView::~ImageView() {
#if !defined(NANOVG_VULKAN_IMPLEMENTATION)
    mShader.free();
#endif
}

void ImageView::bindImage(const GLTexture &texture) {
#if !defined(NANOVG_VULKAN_IMPLEMENTATION)
    mTexture = texture;
    updateImageParameters();
    fit();
#endif
}


void ImageView::draw(NVGcontext* ctx) {
#if defined(NANOVG_VULKAN_IMPLEMENTATION)
    Widget::draw(ctx);
    nvgEndFrame(ctx); // Flush the NanoVG draw stack, not necessary to call nvgBeginFrame afterwards.

    drawWidgetBorder(ctx);
    drawImageBorder(ctx);

    // Calculate several variables that need to be send to OpenGL in order for the image to be
    // properly displayed inside the widget.
    const Screen* screen = dynamic_cast<const Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screenSize = screen->size().cast<float>();
    Vector2f scaleFactor = mScale * imageSizeF().cwiseQuotient(screenSize);
    Vector2f positionInScreen = absolutePosition().cast<float>();
    Vector2f positionAfterOffset = positionInScreen + mOffset;
    Vector2f imagePosition = positionAfterOffset.cwiseQuotient(screenSize);
    glEnable(GL_SCISSOR_TEST);
    float r = screen->pixelRatio();
    glScissor(positionInScreen.x() * r, (screenSize.y() - positionInScreen.y() - size().y()) * r,
              size().x()*r, size().y()*r);
    mShader.bind();
    mTexture.bind();
    mShader.setUniform("image", 0);
    mShader.setUniform("scaleFactor", scaleFactor);
    mShader.setUniform("position", imagePosition);
    mShader.drawIndexed(GL_TRIANGLES, 0, 2);
    mTexture.unbind();
    mShader.unbind();
    glDisable(GL_SCISSOR_TEST);

    if (helpersVisible())
        drawHelpers(ctx);
#endif
}


void ImageView::updateImageParameters() {
    mImageSize = Vector2i(mTexture.width(), mTexture.height());
}

void ImageView::drawWidgetBorder(NVGcontext* ctx) const {
    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0f);
    nvgRoundedRect(ctx, mPos.x() - 0.5f, mPos.y() - 0.5f,
                   mSize.x() + 1, mSize.y() + 1, mTheme->mWindowCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderLight);
    nvgRoundedRect(ctx, mPos.x() - 1.0f, mPos.y() - 1.0f,
                   mSize.x() + 2, mSize.y() + 2, mTheme->mWindowCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderDark);
    nvgStroke(ctx);
}

void ImageView::drawImageBorder(NVGcontext* ctx) const {
    nvgSave(ctx);
    nvgBeginPath(ctx);
    nvgScissor(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
    nvgStrokeWidth(ctx, 1.0f);
    Vector2i borderPosition = mPos + mOffset.cast<int>();
    Vector2i borderSize = scaledImageSizeF().cast<int>();
    nvgRect(ctx, borderPosition.x() - 0.5f, borderPosition.y() - 0.5f,
            borderSize.x() + 1, borderSize.y() + 1);
    nvgStrokeColor(ctx, Color(1.0f, 1.0f, 1.0f, 1.0f));
    nvgStroke(ctx);
    nvgResetScissor(ctx);
    nvgRestore(ctx);
}

void ImageView::drawHelpers(NVGcontext* ctx) const {
    // We need to apply mPos after the transformation to account for the position of the widget
    // relative to the parent.
    Vector2f upperLeftCorner = positionForCoordinate(Vector2f(0, 0)) + positionF();
    Vector2f lowerRightCorner = positionForCoordinate(imageSizeF()) + positionF();
    // Use the scissor method in NanoVG to display only the correct part of the grid.
    Vector2f scissorPosition = upperLeftCorner.array().max(positionF().array());
    Vector2f sizeOffsetDifference = sizeF() - mOffset;
    Vector2f scissorSize = sizeOffsetDifference.array().min(sizeF().array());
    nvgSave(ctx);
    nvgScissor(ctx, scissorPosition.x(), scissorPosition.y(), scissorSize.x(), scissorSize.y());
    if (gridVisible())
        drawPixelGrid(ctx, upperLeftCorner, lowerRightCorner, mScale);
    if (pixelInfoVisible())
        drawPixelInfo(ctx, mScale);
    nvgRestore(ctx);
}

void ImageView::drawPixelGrid(NVGcontext* ctx, const Vector2f& upperLeftCorner,
                              const Vector2f& lowerRightCorner, const float stride) {
    nvgBeginPath(ctx);
    // Draw the vertical lines for the grid
    float currentX = std::floor(upperLeftCorner.x());
    while (currentX <= lowerRightCorner.x()) {
        nvgMoveTo(ctx, std::floor(currentX), std::floor(upperLeftCorner.y()));
        nvgLineTo(ctx, std::floor(currentX), std::floor(lowerRightCorner.y()));
        currentX += stride;
    }
    // Draw the horizontal lines for the grid.
    float currentY = std::floor(upperLeftCorner.y());
    while (currentY <= lowerRightCorner.y()) {
        nvgMoveTo(ctx, std::floor(upperLeftCorner.x()), std::floor(currentY));
        nvgLineTo(ctx, std::floor(lowerRightCorner.x()), std::floor(currentY));
        currentY += stride;
    }
    nvgStrokeWidth(ctx, 1.0f);
    nvgStrokeColor(ctx, Color(1.0f, 1.0f, 1.0f, 1.0f));
    nvgStroke(ctx);
}

void ImageView::drawPixelInfo(NVGcontext* ctx, const float stride) const {
    // Extract the image coordinates at the two corners of the widget.
    Vector2f currentPixelF = clampedImageCoordinateAt(Vector2f::Zero());
    Vector2f lastPixelF = clampedImageCoordinateAt(sizeF());
    // Round the top left coordinates down and bottom down coordinates up.
    // This is done so that the edge information does not pop up suddenly when it gets in range.
    currentPixelF = currentPixelF.unaryExpr([](float x) { return std::floor(x); });
    lastPixelF = lastPixelF.unaryExpr([](float x) { return std::ceil(x); });
    Vector2i currentPixel = currentPixelF.cast<int>();
    Vector2i lastPixel = lastPixelF.cast<int>();

    // Extract the positions for where to draw the text.
    Vector2f currentCellPosition = (positionF() + positionForCoordinate(currentPixelF));
    float xInitialPosition = currentCellPosition.x();
    int xInitialIndex = currentPixel.x();

    // Properly scale the pixel information for the given stride.
    auto fontSize = stride * mFontScaleFactor;
    static constexpr float maxFontSize = 30.0f;
    fontSize = fontSize > maxFontSize ? maxFontSize : fontSize;
    nvgSave(ctx);
    nvgBeginPath(ctx);
    nvgFontSize(ctx, fontSize);
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgFontFace(ctx, "sans");
    while (currentPixel.y() != lastPixel.y()) {
        while (currentPixel.x() != lastPixel.x()) {
            writePixelInfo(ctx, currentCellPosition, currentPixel, stride);
            currentCellPosition.x() += stride;
            ++currentPixel.x();
        }
        currentCellPosition.x() = xInitialPosition;
        currentCellPosition.y() += stride;
        ++currentPixel.y();
        currentPixel.x() = xInitialIndex;
    }
    nvgRestore(ctx);
}

void ImageView::writePixelInfo(NVGcontext* ctx, const Vector2f& cellPosition,
                               const Vector2i& pixel, const float stride) const {
    auto pixelData = mPixelInfoCallback(pixel);
    auto pixelDataRows = splitString(pixelData.first, "\n");

    // If no data is provided for this pixel then simply return.
    if (pixelDataRows.empty())
        return;

    nvgFillColor(ctx, pixelData.second);
    auto padding = stride / 10;
    auto maxSize = stride - 2 * padding;

    // Measure the size of a single line of text.
    float bounds[4];
    nvgTextBoxBounds(ctx, 0.0f, 0.0f, maxSize, pixelDataRows.front().data(), nullptr, bounds);
    auto rowHeight = bounds[3] - bounds[1];
    auto totalRowsHeight = rowHeight * pixelDataRows.size();

    // Choose the initial y offset and the index for the past the last visible row.
    auto yOffset = 0.0f;
    auto lastIndex = 0;

    if (totalRowsHeight > maxSize) {
        yOffset = padding;
        lastIndex = (int) (maxSize / rowHeight);
    } else {
        yOffset = (stride - totalRowsHeight) / 2;
        lastIndex = (int) pixelDataRows.size();
    }

    for (int i = 0; i != lastIndex; ++i) {
        nvgText(ctx, cellPosition.x() + stride / 2, cellPosition.y() + yOffset,
                pixelDataRows[i].data(), nullptr);
        yOffset += rowHeight;
    }
}

NAMESPACE_END(nanogui)

#endif