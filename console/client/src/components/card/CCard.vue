<template>
  <div class="card" :class="{ [variant]: variant }">
    <slot name="cardTop"></slot>
    <div class="card-head">
      <slot name="header"> </slot>
    </div>

    <p>
      <slot name="content"></slot>
    </p>
    <div class="card-footer">{{ cardData }}</div>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  props: ["cardData", "variant"]
});
</script>