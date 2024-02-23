/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { HippyAny, HippyMap, HippyRenderCallback } from '../../../support/common/HippyTypes';
import { HRComponent } from '../../HippyRenderView';
import { NativeRenderContext } from '../../NativeRenderContext';
import { HREventUtils } from '../../utils/HREventUtils'
import HippyRenderBaseView, {
  HippyObservedArray } from '../base/HippyRenderBaseView';
import { LogUtils } from '../../../support/utils/LogUtils';

@Observed
export class HRWaterfallView extends HippyRenderBaseView {
  private readonly TAG = "HRWaterfallView"
  public flex: number = 0
  public containPullFooter: boolean = false
  public contentInset: number = 0;
  public containPullHeader: boolean = false
  public containBannerView: boolean = true
  public scrollEventThrottle: number = 200
  public preloadItemNumber: number = 0
  public iterItemSpacing: number = 0
  public columnSpacing: number = 0
  public numberOfColumns: number = 0
  public columnsTemplate: string = '1fr 1fr'
  public x: number = 0
  public y: number = 0

  constructor(ctx: NativeRenderContext) {
    super(ctx)
  }

  setProp(propKey: string, propValue: HippyAny | HippyRenderCallback): boolean {
    LogUtils.d(this.TAG, "setProp: propKey " + propKey + ", value: " + propValue)
    switch (propKey) {
      case 'flex':
        this.flex = propValue as number;
        break;
      case 'containPullFooter':
        this.containPullFooter = propValue as boolean;
        break;
      case 'contentInset':
        this.contentInset = propValue as number;
        break;
      case 'containPullHeader':
        this.containPullHeader = propValue as boolean;
        break;
      case 'containBannerView':
        this.containBannerView = propValue as boolean;
        break;
      case 'scrollEventThrottle':
        this.scrollEventThrottle = propValue as number;
        break;
      case 'preloadItemNumber':
        this.preloadItemNumber = propValue as number;
        break;
      case 'iterItemSpacing':
        this.iterItemSpacing = propValue as number;
        break;
      case 'columnSpacing':
        this.columnSpacing = propValue as number;
        break;
      case 'numberOfColumns':
        this.numberOfColumns = propValue as number;
        break;
      default:
      break;
    }

    return super.setProp(propKey, propValue as HippyAny)
  }
}

@Component
export struct HRWaterfall {
  private readonly TAG = "HRWaterfall"
  @ObjectLink renderView: HRWaterfallView
  @ObjectLink children: HippyObservedArray<HippyRenderBaseView>
  @BuilderParam buildRenderView: ($$: HippyRenderBaseView, parent: HRComponent) => void
  private PAGE_ITEM_POSITION: string = "position";
  @State index: number = 0

  handleChangeEvent(index: number) {
    LogUtils.d(this.TAG, "handleChangeEvent: index " + index)
    let params = new Map<string, number>() as HippyMap;
    params.set(this.PAGE_ITEM_POSITION, index);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_SELECTED, params);
  }

  handleGestureSwipeEvent(index: number, extraInfo: SwiperAnimationEvent) {
    LogUtils.d(this.TAG, "handleGestureSwipeEvent: index " + index)
    let params = new Map<string, Number>() as HippyMap;
    params.set(this.PAGE_ITEM_POSITION, index);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_SELECTED, params);
  }

  handleAnimationStartEvent(index: number, targetIndex: number, extraInfo: SwiperAnimationEvent) {
    LogUtils.d(this.TAG, "handleAnimationStartEvent: index " + targetIndex)
    let params = new Map<string, Number>() as HippyMap;
    params.set(this.PAGE_ITEM_POSITION, targetIndex);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_SELECTED, params);
    params.set(this.PAGE_ITEM_POSITION, targetIndex);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_ITEM_WILL_APPEAR, params);
    params.set(this.PAGE_ITEM_POSITION, index);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_ITEM_WILL_DISAPPEAR, params);
  }

  handleAnimationEnd(index: number, extraInfo: SwiperAnimationEvent) {
    LogUtils.d(this.TAG, "handleAnimationEnd: index " + index)
    let params = new Map<string, Number>() as HippyMap;
    params.set(this.PAGE_ITEM_POSITION, index);
    HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_SELECTED, params);
  }

  aboutToAppear() {
    LogUtils.d(this.TAG, "aboutToAppear ")
  }

  aboutToDisappear() {
  }

  setPage(index: number) {
    this.index = index
  }

  build() {
    Scroll() {
      Column({ space: 2 }) {
        ForEach(this.children, (item: HippyRenderBaseView) => {
          if (item.viewType === 'View') {
            Stack() {
              this.buildRenderView(item, null)
            }
          }
        }, (item: HippyRenderBaseView) => item.tag + '')
        WaterFlow() {
          ForEach(this.children, (item: HippyRenderBaseView) => {
            if (item.viewType === 'WaterfallItem') {
              FlowItem() {
                this.buildRenderView(item, null)
              }
            }
          }, (item: HippyRenderBaseView) => item.tag + '')
        }
        .columnsTemplate("1fr 1fr")
        .columnsGap(10)
        .rowsGap(5)
        .width('100%')
        .height('100%')
        .nestedScroll({
          scrollForward: NestedScrollMode.PARENT_FIRST,
          scrollBackward: NestedScrollMode.PARENT_FIRST,})
      }
    }
    .applyRenderViewBaseAttr(this.renderView)
    .width('100%')
    .height('100%')
    .position({x:this.renderView.x, y: this.renderView.y})
    .scrollBar(BarState.Off)
    .edgeEffect(EdgeEffect.Spring)
    .onScroll((xOffset:number, yOffset:number) => {
      LogUtils.d(this.TAG, "onScroll: xOffset:" + xOffset + " yOffset:" + yOffset)
      if (xOffset === 0 && yOffset === 0) {
        this.renderView.x = 0
        this.renderView.y = 0
        HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag,
          HREventUtils.EVENT_REFRESH_WRAPPER_REFRESH, null);
      }
    })
    .onScrollEdge((side: Edge) => {
      LogUtils.d(this.TAG, "onScrollEdge: side " + side)
      if (side === Edge.Top) {
        this.renderView.x = 0
        this.renderView.y = 0
        HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag,
          HREventUtils.EVENT_REFRESH_WRAPPER_REFRESH, null);
      }
      // HREventUtils.sendComponentEvent(this.renderView.ctx, this.renderView.tag, HREventUtils.EVENT_PAGE_SELECTED, params);
    })
  }
}

// base props for all components
@Extend(Scroll)
function applyRenderViewBaseAttr($$: HippyRenderBaseView) {
  .backgroundColor($$.cssBackgroundColor)
  // .position({ x: $$.cssPositionX, y: $$.cssPositionY })
  .size({ width: $$.cssWidth, height: $$.cssHeight })
  .opacity($$.cssOpacity)
  .clip($$.cssOverflow)
  .visibility(($$ as HippyRenderBaseView).cssVisibility) // must add as, otherwise the compiler has error
  .zIndex($$.cssZIndex)
  .border($$.cssBorder)
  .borderWidth(2) // TODO:
  .rotate($$.cssRotate)
  .scale($$.cssScale)
  .translate($$.cssTranslate)
  .onClick($$.eventClick)
}